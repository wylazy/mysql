/* Copyright (c) 2004, 2013, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/**
  @file ha_spartan.cc

  @brief
  The ha_spartan engine is a stubbed storage engine for spartan purposes only;
  it does nothing at this point. Its purpose is to provide a source
  code illustration of how to begin writing new storage engines; see also
  /storage/spartan/ha_spartan.h.

  @details
  ha_spartan will let you create/open/delete tables, but
  nothing further (for spartan, indexes are not supported nor can data
  be stored in the table). Use this spartan as a template for
  implementing the same functionality in your own storage engine. You
  can enable the spartan storage engine in your build by doing the
  following during your build process:<br> ./configure
  --with-spartan-storage-engine

  Once this is done, MySQL will let you create tables with:<br>
  CREATE TABLE <table name> (...) ENGINE=SPARTAN;

  The spartan storage engine is set up to use table locks. It
  implements an spartan "SHARE" that is inserted into a hash by table
  name. You can use this to store information of state that any
  spartan handler object will be able to see when it is using that
  table.

  Please read the object definition in ha_spartan.h before reading the rest
  of this file.

  @note
  When you create an SPARTAN table, the MySQL Server creates a table .frm
  (format) file in the database directory, using the table name as the file
  name as is customary with MySQL. No other files are created. To get an idea
  of what occurs, here is an spartan select that would do a scan of an entire
table:

@code
ha_spartan::store_lock
ha_spartan::external_lock
ha_spartan::info
ha_spartan::rnd_init
ha_spartan::extra
ENUM HA_EXTRA_CACHE        Cache record in HA_rrnd()
ha_spartan::rnd_next
ha_spartan::rnd_next
ha_spartan::rnd_next
ha_spartan::rnd_next
ha_spartan::rnd_next
ha_spartan::rnd_next
ha_spartan::rnd_next
ha_spartan::rnd_next
ha_spartan::rnd_next
ha_spartan::extra
ENUM HA_EXTRA_NO_CACHE     End caching of records (def)
ha_spartan::external_lock
ha_spartan::extra
ENUM HA_EXTRA_RESET        Reset database to after open
@endcode

Here you see that the spartan storage engine has 9 rows called before
rnd_next signals that it has reached the end of its data. Also note that
the table in question was already opened; had it not been open, a call to
ha_spartan::open() would also have been necessary. Calls to
ha_spartan::extra() are hints as to what will be occuring to the request.

A Longer Spartan can be found called the "Skeleton Engine" which can be 
found on TangentOrg. It has both an engine and a full build environment
for building a pluggable storage engine.

Happy coding!<br>
-Brian
*/

#include "sql_priv.h"
#include "sql_class.h"           // MYSQL_HANDLERTON_INTERFACE_VERSION
#include "ha_spartan.h"
#include "probes_mysql.h"
#include "sql_plugin.h"


#define SPD_EXT ".spd"               // The data file

static handler *spartan_create_handler(handlerton *hton,
    TABLE_SHARE *table, 
    MEM_ROOT *mem_root);

handlerton *spartan_hton;

/* Interface to mysqld, to check system tables supported by SE */
static const char* spartan_system_database();
static bool spartan_is_supported_system_table(const char *db,
    const char *table_name,
    bool is_sql_layer_system_table);

#ifdef HAVE_PSI_INTERFACE
static PSI_mutex_key ex_key_mutex_Spartan_share_mutex;

static PSI_mutex_info all_spartan_mutexes[]=
{
  { &ex_key_mutex_Spartan_share_mutex, "Spartan_share::mutex", 0}
};

static PSI_file_key spd_file_data;

static PSI_file_info all_tina_files[]=
{
  { &spd_file_data, "data", 0},
};

static void init_spartan_psi_keys()
{
  const char* category= "spartan";
  int count;

  count= array_elements(all_spartan_mutexes);
  mysql_mutex_register(category, all_spartan_mutexes, count);

  count= array_elements(all_tina_files);
  mysql_file_register(category, all_tina_files, count);
}
#endif

Spartan_share::Spartan_share()
{
  thr_lock_init(&lock);
  mysql_mutex_init(ex_key_mutex_Spartan_share_mutex,
      &mutex, MY_MUTEX_INIT_FAST);
}


static int spartan_init_func(void *p)
{
  DBUG_ENTER("spartan_init_func");

#ifdef HAVE_PSI_INTERFACE
  init_spartan_psi_keys();
#endif

  spartan_hton= (handlerton *)p;
  spartan_hton->state=                     SHOW_OPTION_YES;
  spartan_hton->create=                    spartan_create_handler;
  spartan_hton->flags= (HTON_CAN_RECREATE | HTON_SUPPORT_LOG_TABLES | HTON_NO_PARTITION);
  spartan_hton->system_database=   spartan_system_database;
  spartan_hton->is_supported_system_table= spartan_is_supported_system_table;

  DBUG_RETURN(0);
}


/**
  @brief
  Spartan of simple lock controls. The "share" it creates is a
  structure we will pass to each spartan handler. Do you have to have
  one of these? Well, you have pieces that are used for locking, and
  they are needed to function.
  */

Spartan_share *ha_spartan::get_share(const char *table_name, TABLE *table)
{
  Spartan_share *tmp_share;

  DBUG_ENTER("ha_spartan::get_share()");

  lock_shared_ha_data();
  if (!(tmp_share= static_cast<Spartan_share*>(get_ha_share_ptr())))
  {
    tmp_share= new Spartan_share;
    if (!tmp_share)
      goto err;

    set_ha_share_ptr(static_cast<Handler_share*>(tmp_share));
    strmov(tmp_share->table_name, table_name);
    fn_format(tmp_share->data_file_name, table_name, "", SPD_EXT,
        MY_REPLACE_EXT|MY_UNPACK_FILENAME);
  }

err:
  unlock_shared_ha_data();
  DBUG_RETURN(tmp_share);
}


static handler* spartan_create_handler(handlerton *hton,
    TABLE_SHARE *table, 
    MEM_ROOT *mem_root)
{
  return new (mem_root) ha_spartan(hton, table);
}

  ha_spartan::ha_spartan(handlerton *hton, TABLE_SHARE *table_arg)
:handler(hton, table_arg)
{}


/**
  @brief
  If frm_error() is called then we will use this to determine
  the file extensions that exist for the storage engine. This is also
  used by the default rename_table and delete_table method in
  handler.cc.

  For engines that have two file name extentions (separate meta/index file
  and data file), the order of elements is relevant. First element of engine
  file name extentions array should be meta/index file extention. Second
  element - data file extention. This order is assumed by
  prepare_for_repair() when REPAIR TABLE ... USE_FRM is issued.

  @see
  rename_table method in handler.cc and
  delete_table method in handler.cc
  */

static const char *ha_spartan_exts[] = {
  SPD_EXT,
  NullS
};

const char **ha_spartan::bas_ext() const
{
  return ha_spartan_exts;
}

/*
   Following handler function provides access to
   system database specific to SE. This interface
   is optional, so every SE need not implement it.
   */
const char* ha_spartan_system_database= NULL;
const char* spartan_system_database()
{
  return ha_spartan_system_database;
}

/*
   List of all system tables specific to the SE.
   Array element would look like below,
   { "<database_name>", "<system table name>" },
   The last element MUST be,
   { (const char*)NULL, (const char*)NULL }

   This array is optional, so every SE need not implement it.
   */
static st_system_tablename ha_spartan_system_tables[]= {
  {(const char*)NULL, (const char*)NULL}
};

/**
  @brief Check if the given db.tablename is a system table for this SE.

  @param db                         Database name to check.
  @param table_name                 table name to check.
  @param is_sql_layer_system_table  if the supplied db.table_name is a SQL
  layer system table.

  @return
  @retval TRUE   Given db.table_name is supported system table.
  @retval FALSE  Given db.table_name is not a supported system table.
  */
static bool spartan_is_supported_system_table(const char *db,
    const char *table_name,
    bool is_sql_layer_system_table)
{
  st_system_tablename *systab;

  // Does this SE support "ALL" SQL layer system tables ?
  if (is_sql_layer_system_table)
    return false;

  // Check if this is SE layer system tables
  systab= ha_spartan_system_tables;
  while (systab && systab->db)
  {
    if (systab->db == db &&
        strcmp(systab->tablename, table_name) == 0)
      return true;
    systab++;
  }

  return false;
}


/**
  @brief
  Used for opening tables. The name will be the name of the file.

  @details
  A table is opened when it needs to be opened; e.g. when a request comes in
  for a SELECT on the table (tables are not open and closed for each request,
  they are cached).

  Called from handler.cc by handler::ha_open(). The server opens all tables by
  calling ha_open() which then calls the handler specific open().

  @see
  handler::ha_open() in handler.cc
  */

int ha_spartan::open(const char *name, int mode, uint test_if_locked)
{
  DBUG_ENTER("ha_spartan::open");

  if (!(share = get_share(name, table)))
    DBUG_RETURN(HA_ERR_OUT_OF_MEM);

  if ((data_file= mysql_file_open(spd_file_data, share->data_file_name, O_RDWR, MYF(MY_WME))) == -1)
  {
    DBUG_RETURN(my_errno ? my_errno : -1);
  }

  thr_lock_data_init(&share->lock,&lock, (void *)this);
  io_size = table->s->reclength;
  io_buf = (uchar *)malloc(io_size);
  if (io_buf == NULL) {
    io_size = 0;
    mysql_file_close(data_file, MYF(MY_WME));
    DBUG_RETURN(HA_ERR_OUT_OF_MEM);
  }

  DBUG_RETURN(0);
}


/**
  @brief
  Closes a table.

  @details
  Called from sql_base.cc, sql_select.cc, and table.cc. In sql_select.cc it is
  only used to close up temporary tables or during the process where a
  temporary table is converted over to being a myisam table.

  For sql_base.cc look at close_data_tables().

  @see
  sql_base.cc, sql_select.cc and table.cc
  */

int ha_spartan::close(void)
{
  int rc = 0;
  DBUG_ENTER("ha_spartan::close");
  if (io_buf) {
    free(io_buf);
  }
  rc= mysql_file_close(data_file, MYF(0));
  data_file = -1;
  DBUG_RETURN(rc);
}


/**
  @brief
  write_row() inserts a row. No extra() hint is given currently if a bulk load
  is happening. buf() is a byte array of data. You can use the field
  information to extract the data from the native byte array type.

  @details
  Spartan of this would be:
  @code
  for (Field **field=table->field ; *field ; field++)
  {
  ...
  }
  @endcode

  See ha_tina.cc for an spartan of extracting all of the data as strings.
  ha_berekly.cc has an spartan of how to store it intact by "packing" it
  for ha_berkeley's own native storage type.

  See the note for update_row() on auto_increments. This case also applies to
  write_row().

  Called from item_sum.cc, item_sum.cc, sql_acl.cc, sql_insert.cc,
  sql_insert.cc, sql_select.cc, sql_table.cc, sql_udf.cc, and sql_update.cc.

  @see
  item_sum.cc, item_sum.cc, sql_acl.cc, sql_insert.cc,
  sql_insert.cc, sql_select.cc, sql_table.cc, sql_udf.cc and sql_update.cc
  */

int ha_spartan::write_row(uchar *buf)
{
  DBUG_ENTER("ha_spartan::write_row");

  if (encode_buf(buf)) {
    return HA_ERR_UNSUPPORTED;
  }

  if (fcntl(data_file, F_SETFL, O_APPEND)) {
    DBUG_RETURN(HA_ERR_INTERNAL_ERROR);
  }

   /* use pwrite, as concurrent reader could have changed the position */
  if (mysql_file_write(data_file, (uchar*)buffer.ptr(), buffer.length(), MYF(MY_WME | MY_NABP)))
    DBUG_RETURN(HA_ERR_INTERNAL_ERROR);

  DBUG_RETURN(0);
}


/**
  @brief
  Yes, update_row() does what you expect, it updates a row. old_data will have
  the previous row record in it, while new_data will have the newest data in it.
  Keep in mind that the server can do updates based on ordering if an ORDER BY
  clause was used. Consecutive ordering is not guaranteed.

  @details
  Currently new_data will not have an updated auto_increament record. You can
  do this for spartan by doing:

  @code

  if (table->next_number_field && record == table->record[0])
  update_auto_increment();

  @endcode

  Called from sql_select.cc, sql_acl.cc, sql_update.cc, and sql_insert.cc.

  @see
  sql_select.cc, sql_acl.cc, sql_update.cc and sql_insert.cc
  */
int ha_spartan::update_row(const uchar *old_data, uchar *new_data)
{

  DBUG_ENTER("ha_spartan::update_row");

  if (encode_buf(new_data)) {
    return HA_ERR_UNSUPPORTED;
  }
  fcntl(data_file, F_SETFL, O_RDWR);

  lseek(data_file, current_position, SEEK_SET);
   /* use pwrite, as concurrent reader could have changed the position */
  if (mysql_file_write(data_file, (uchar*)buffer.ptr(), buffer.length(), MYF(MY_WME | MY_NABP)))
    DBUG_RETURN(HA_ERR_INTERNAL_ERROR);

  DBUG_RETURN(0);
}


/**
  @brief
  This will delete a row. buf will contain a copy of the row to be deleted.
  The server will call this right after the current row has been called (from
  either a previous rnd_nexT() or index call).

  @details
  If you keep a pointer to the last row or can access a primary key it will
  make doing the deletion quite a bit easier. Keep in mind that the server does
  not guarantee consecutive deletions. ORDER BY clauses can be used.

  Called in sql_acl.cc and sql_udf.cc to manage internal table
  information.  Called in sql_delete.cc, sql_insert.cc, and
  sql_select.cc. In sql_select it is used for removing duplicates
  while in insert it is used for REPLACE calls.

  @see
  sql_acl.cc, sql_udf.cc, sql_delete.cc, sql_insert.cc and sql_select.cc
  */

int ha_spartan::delete_row(const uchar *buf)
{

  int len = 0;
  my_off_t old_cur = current_position;
  DBUG_ENTER("ha_spartan::delete_row");

  fcntl(data_file, F_SETFL, O_RDWR);

  //read from next_position to eof
  while ((len = read_one_record()) > 0) {
    lseek(data_file, current_position, SEEK_SET); 
    if (mysql_file_write(data_file, (uchar*)io_buf, len, MYF(MY_WME | MY_NABP)))
      DBUG_RETURN(HA_ERR_INTERNAL_ERROR);

    DBUG_ASSERT(next_position - current_position == len);
    current_position += len;
    next_position += len;
    lseek(data_file, current_position+len, SEEK_SET); 
  }


  if (ftruncate(data_file, current_position)) {
    DBUG_RETURN(HA_ERR_INTERNAL_ERROR);
  }

  current_position = next_position = old_cur;
  lseek(data_file, current_position, SEEK_SET);
  DBUG_RETURN(0);
}


/**
  @brief
  Positions an index cursor to the index specified in the handle. Fetches the
  row if available. If the key value is null, begin at the first key of the
  index.
  */

int ha_spartan::index_read_map(uchar *buf, const uchar *key,
    key_part_map keypart_map __attribute__((unused)),
    enum ha_rkey_function find_flag
    __attribute__((unused)))
{
  int rc;
  DBUG_ENTER("ha_spartan::index_read");
  MYSQL_INDEX_READ_ROW_START(table_share->db.str, table_share->table_name.str);
  rc= HA_ERR_WRONG_COMMAND;
  MYSQL_INDEX_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  Used to read forward through the index.
  */

int ha_spartan::index_next(uchar *buf)
{
  int rc;
  DBUG_ENTER("ha_spartan::index_next");
  MYSQL_INDEX_READ_ROW_START(table_share->db.str, table_share->table_name.str);
  rc= HA_ERR_WRONG_COMMAND;
  MYSQL_INDEX_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  Used to read backwards through the index.
  */

int ha_spartan::index_prev(uchar *buf)
{
  int rc;
  DBUG_ENTER("ha_spartan::index_prev");
  MYSQL_INDEX_READ_ROW_START(table_share->db.str, table_share->table_name.str);
  rc= HA_ERR_WRONG_COMMAND;
  MYSQL_INDEX_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  index_first() asks for the first key in the index.

  @details
  Called from opt_range.cc, opt_sum.cc, sql_handler.cc, and sql_select.cc.

  @see
  opt_range.cc, opt_sum.cc, sql_handler.cc and sql_select.cc
  */
int ha_spartan::index_first(uchar *buf)
{
  int rc;
  DBUG_ENTER("ha_spartan::index_first");
  MYSQL_INDEX_READ_ROW_START(table_share->db.str, table_share->table_name.str);
  rc= HA_ERR_WRONG_COMMAND;
  MYSQL_INDEX_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  index_last() asks for the last key in the index.

  @details
  Called from opt_range.cc, opt_sum.cc, sql_handler.cc, and sql_select.cc.

  @see
  opt_range.cc, opt_sum.cc, sql_handler.cc and sql_select.cc
  */
int ha_spartan::index_last(uchar *buf)
{
  int rc;
  DBUG_ENTER("ha_spartan::index_last");
  MYSQL_INDEX_READ_ROW_START(table_share->db.str, table_share->table_name.str);
  rc= HA_ERR_WRONG_COMMAND;
  MYSQL_INDEX_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  rnd_init() is called when the system wants the storage engine to do a table
  scan. See the spartan in the introduction at the top of this file to see when
  rnd_init() is called.

  @details
  Called from filesort.cc, records.cc, sql_handler.cc, sql_select.cc, sql_table.cc,
  and sql_update.cc.

  @see
  filesort.cc, records.cc, sql_handler.cc, sql_select.cc, sql_table.cc and sql_update.cc
  */
int ha_spartan::rnd_init(bool scan)
{
  DBUG_ENTER("ha_spartan::rnd_init");
  current_position= next_position= 0;
  fcntl(data_file, F_SETFL, O_RDONLY);
  lseek(data_file, 0, SEEK_SET);
  DBUG_RETURN(0);
}

int ha_spartan::rnd_end()
{
  DBUG_ENTER("ha_spartan::rnd_end");
  free_root(&blobroot, MYF(0));
  DBUG_RETURN(0);
}


/**
  @brief
  This is called for each row of the table scan. When you run out of records
  you should return HA_ERR_END_OF_FILE. Fill buff up with the row information.
  The Field structure for the table is the key to getting data into buf
  in a manner that will allow the server to understand it.

  @details
  Called from filesort.cc, records.cc, sql_handler.cc, sql_select.cc, sql_table.cc,
  and sql_update.cc.

  @see
  filesort.cc, records.cc, sql_handler.cc, sql_select.cc, sql_table.cc and sql_update.cc
  */
int ha_spartan::rnd_next(uchar *buf)
{
  int rc;
  DBUG_ENTER("ha_spartan::rnd_next");
  MYSQL_READ_ROW_START(table_share->db.str, table_share->table_name.str, TRUE);

  current_position= next_position;
  rc = find_current_row(buf);
  next_position = lseek(data_file, 0, SEEK_CUR);

  if (rc) {
    goto end;
  }

end:
  MYSQL_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  position() is called after each call to rnd_next() if the data needs
  to be ordered. You can do something like the following to store
  the position:
  @code
  my_store_ptr(ref, ref_length, current_position);
  @endcode

  @details
  The server uses ref to store data. ref_length in the above case is
  the size needed to store current_position. ref is just a byte array
  that the server will maintain. If you are using offsets to mark rows, then
  current_position should be the offset. If it is a primary key like in
  BDB, then it needs to be a primary key.

  Called from filesort.cc, sql_select.cc, sql_delete.cc, and sql_update.cc.

  @see
  filesort.cc, sql_select.cc, sql_delete.cc and sql_update.cc
  */
void ha_spartan::position(const uchar *record)
{
  DBUG_ENTER("ha_spartan::position");
  my_store_ptr(ref, ref_length, current_position);
  DBUG_VOID_RETURN;
}


/**
  @brief
  This is like rnd_next, but you are given a position to use
  to determine the row. The position will be of the type that you stored in
  ref. You can use ha_get_ptr(pos,ref_length) to retrieve whatever key
  or position you saved when position() was called.

  @details
  Called from filesort.cc, records.cc, sql_insert.cc, sql_select.cc, and sql_update.cc.

  @see
  filesort.cc, records.cc, sql_insert.cc, sql_select.cc and sql_update.cc
  */
int ha_spartan::rnd_pos(uchar *buf, uchar *pos)
{
  int rc;
  DBUG_ENTER("ha_spartan::rnd_pos");
  MYSQL_READ_ROW_START(table_share->db.str, table_share->table_name.str, FALSE);
  current_position= my_get_ptr(pos,ref_length);
  rc= find_current_row(buf);
  MYSQL_READ_ROW_DONE(rc);
  DBUG_RETURN(rc);
}


/**
  @brief
  ::info() is used to return information to the optimizer. See my_base.h for
  the complete description.

  @details
  Currently this table handler doesn't implement most of the fields really needed.
  SHOW also makes use of this data.

  You will probably want to have the following in your code:
  @code
  if (records < 2)
  records = 2;
  @endcode
  The reason is that the server will optimize for cases of only a single
  record. If, in a table scan, you don't know the number of records, it
  will probably be better to set records to two so you can return as many
  records as you need. Along with records, a few more variables you may wish
  to set are:
  records
  deleted
  data_file_length
  index_file_length
  delete_length
  check_time
  Take a look at the public variables in handler.h for more information.

  Called in filesort.cc, ha_heap.cc, item_sum.cc, opt_sum.cc, sql_delete.cc,
  sql_delete.cc, sql_derived.cc, sql_select.cc, sql_select.cc, sql_select.cc,
  sql_select.cc, sql_select.cc, sql_show.cc, sql_show.cc, sql_show.cc, sql_show.cc,
  sql_table.cc, sql_union.cc, and sql_update.cc.

  @see
  filesort.cc, ha_heap.cc, item_sum.cc, opt_sum.cc, sql_delete.cc, sql_delete.cc,
  sql_derived.cc, sql_select.cc, sql_select.cc, sql_select.cc, sql_select.cc,
  sql_select.cc, sql_show.cc, sql_show.cc, sql_show.cc, sql_show.cc, sql_table.cc,
  sql_union.cc and sql_update.cc
  */
int ha_spartan::info(uint flag)
{
  DBUG_ENTER("ha_spartan::info");
  DBUG_RETURN(0);
}


/**
  @brief
  extra() is called whenever the server wishes to send a hint to
  the storage engine. The myisam engine implements the most hints.
  ha_innodb.cc has the most exhaustive list of these hints.

  @see
  ha_innodb.cc
  */
int ha_spartan::extra(enum ha_extra_function operation)
{
  DBUG_ENTER("ha_spartan::extra");
  DBUG_RETURN(0);
}


/**
  @brief
  Used to delete all rows in a table, including cases of truncate and cases where
  the optimizer realizes that all rows will be removed as a result of an SQL statement.

  @details
  Called from item_sum.cc by Item_func_group_concat::clear(),
  Item_sum_count_distinct::clear(), and Item_func_group_concat::clear().
  Called from sql_delete.cc by mysql_delete().
  Called from sql_select.cc by JOIN::reinit().
  Called from sql_union.cc by st_select_lex_unit::exec().

  @see
  Item_func_group_concat::clear(), Item_sum_count_distinct::clear() and
  Item_func_group_concat::clear() in item_sum.cc;
  mysql_delete() in sql_delete.cc;
  JOIN::reinit() in sql_select.cc and
  st_select_lex_unit::exec() in sql_union.cc.
  */
int ha_spartan::delete_all_rows()
{
  DBUG_ENTER("ha_spartan::delete_all_rows");
  DBUG_RETURN(HA_ERR_WRONG_COMMAND);
}


/**
  @brief
  Used for handler specific truncate table.  The table is locked in
  exclusive mode and handler is responsible for reseting the auto-
  increment counter.

  @details
  Called from Truncate_statement::handler_truncate.
  Not used if the handlerton supports HTON_CAN_RECREATE, unless this
  engine can be used as a partition. In this case, it is invoked when
  a particular partition is to be truncated.

  @see
  Truncate_statement in sql_truncate.cc
  Remarks in handler::truncate.
  */
int ha_spartan::truncate()
{
  DBUG_ENTER("ha_spartan::truncate");
  DBUG_RETURN(HA_ERR_WRONG_COMMAND);
}


/**
  @brief
  This create a lock on the table. If you are implementing a storage engine
  that can handle transacations look at ha_berkely.cc to see how you will
  want to go about doing this. Otherwise you should consider calling flock()
  here. Hint: Read the section "locking functions for mysql" in lock.cc to understand
  this.

  @details
  Called from lock.cc by lock_external() and unlock_external(). Also called
  from sql_table.cc by copy_data_between_tables().

  @see
  lock.cc by lock_external() and unlock_external() in lock.cc;
  the section "locking functions for mysql" in lock.cc;
  copy_data_between_tables() in sql_table.cc.
  */
int ha_spartan::external_lock(THD *thd, int lock_type)
{
  DBUG_ENTER("ha_spartan::external_lock");
  DBUG_RETURN(0);
}


/**
  @brief
  The idea with handler::store_lock() is: The statement decides which locks
  should be needed for the table. For updates/deletes/inserts we get WRITE
  locks, for SELECT... we get read locks.

  @details
  Before adding the lock into the table lock handler (see thr_lock.c),
  mysqld calls store lock with the requested locks. Store lock can now
  modify a write lock to a read lock (or some other lock), ignore the
  lock (if we don't want to use MySQL table locks at all), or add locks
  for many tables (like we do when we are using a MERGE handler).

  Berkeley DB, for spartan, changes all WRITE locks to TL_WRITE_ALLOW_WRITE
  (which signals that we are doing WRITES, but are still allowing other
  readers and writers).

  When releasing locks, store_lock() is also called. In this case one
  usually doesn't have to do anything.

  In some exceptional cases MySQL may send a request for a TL_IGNORE;
  This means that we are requesting the same lock as last time and this
  should also be ignored. (This may happen when someone does a flush
  table when we have opened a part of the tables, in which case mysqld
  closes and reopens the tables and tries to get the same locks at last
  time). In the future we will probably try to remove this.

  Called from lock.cc by get_lock_data().

  @note
  In this method one should NEVER rely on table->in_use, it may, in fact,
  refer to a different thread! (this happens if get_lock_data() is called
  from mysql_lock_abort_for_thread() function)

  @see
  get_lock_data() in lock.cc
  */
THR_LOCK_DATA **ha_spartan::store_lock(THD *thd,
    THR_LOCK_DATA **to,
    enum thr_lock_type lock_type)
{
  if (lock_type != TL_IGNORE && lock.type == TL_UNLOCK)
    lock.type=lock_type;
  *to++= &lock;
  return to;
}


/**
  @brief
  Used to delete a table. By the time delete_table() has been called all
  opened references to this table will have been closed (and your globally
  shared references released). The variable name will just be the name of
  the table. You will need to remove any files you have created at this point.

  @details
  If you do not implement this, the default delete_table() is called from
  handler.cc and it will delete all files with the file extensions returned
  by bas_ext().

  Called from handler.cc by delete_table and ha_create_table(). Only used
  during create if the table_flag HA_DROP_BEFORE_CREATE was specified for
  the storage engine.

  @see
  delete_table and ha_create_table() in handler.cc
  */
int ha_spartan::delete_table(const char *name)
{
  DBUG_ENTER("ha_spartan::delete_table");
  /* This is not implemented but we want someone to be able that it works. */
  DBUG_RETURN(0);
}


/**
  @brief
  Renames a table from one name to another via an alter table call.

  @details
  If you do not implement this, the default rename_table() is called from
  handler.cc and it will delete all files with the file extensions returned
  by bas_ext().

  Called from sql_table.cc by mysql_rename_table().

  @see
  mysql_rename_table() in sql_table.cc
  */
int ha_spartan::rename_table(const char * from, const char * to)
{
  DBUG_ENTER("ha_spartan::rename_table ");
  DBUG_RETURN(HA_ERR_WRONG_COMMAND);
}


/**
  @brief
  Given a starting key and an ending key, estimate the number of rows that
  will exist between the two keys.

  @details
  end_key may be empty, in which case determine if start_key matches any rows.

  Called from opt_range.cc by check_quick_keys().

  @see
  check_quick_keys() in opt_range.cc
  */
ha_rows ha_spartan::records_in_range(uint inx, key_range *min_key,
    key_range *max_key)
{
  DBUG_ENTER("ha_spartan::records_in_range");
  DBUG_RETURN(10);                         // low number to force index usage
}


/**
  @brief
  create() is called to create a database. The variable name will have the name
  of the table.

  @details
  When create() is called you do not need to worry about
  opening the table. Also, the .frm file will have already been
  created so adjusting create_info is not necessary. You can overwrite
  the .frm file at this point if you wish to change the table
  definition, but there are no methods currently provided for doing
  so.

  Called from handle.cc by ha_create_table().

  @see
  ha_create_table() in handle.cc
  */

int ha_spartan::create(const char *name, TABLE *table_arg,
    HA_CREATE_INFO *create_info)
{
  File create_file;
  char name_buff[FN_REFLEN];

  DBUG_ENTER("ha_spartan::create");

  /*
     check columns
     */
  for (Field **field= table_arg->s->field; *field; field++)
  {
    if ((*field)->real_maybe_null())
    {
      my_error(ER_CHECK_NOT_IMPLEMENTED, MYF(0), "nullable columns");
      DBUG_RETURN(HA_ERR_UNSUPPORTED);
    }
  }

  if ((create_file= mysql_file_create(spd_file_data, fn_format(name_buff, name, "", SPD_EXT, MY_REPLACE_EXT|MY_UNPACK_FILENAME), 0, O_RDWR | O_TRUNC, MYF(MY_WME))) < 0)
    DBUG_RETURN(-1);

  mysql_file_close(create_file, MYF(0));
  DBUG_RETURN(0);
}


/**
 * read one record to io_buf
 * return >0: Successfully
 * return =0: Read EOF
 * return <0: Error
 */
int ha_spartan::read_one_record() {

  int cur_len = 0;
  unsigned long total_len = 0;
  while (total_len < io_size && (cur_len = my_read(data_file, io_buf + total_len, io_size - total_len, 0)) > 0) {
    total_len += cur_len;
  }

  DBUG_ASSERT(total_len <= io_size);

  if (total_len == io_size) {
    return total_len;
  } else if (cur_len == 0) {
    return 0;
  } else {
    return -1;
  }
}

int ha_spartan::encode_buf(uchar *buf) {

  size_t i = 0;
  uint tmp_int;
  ulonglong tmp_long;
  char attribute_buffer[1024];
  String attribute(attribute_buffer, sizeof(attribute_buffer),
      &my_charset_bin);

  buffer.length(0);
  my_bitmap_map *org_bitmap= dbug_tmp_use_all_columns(table, table->read_set);

  for (Field **field=table->field ; *field ; field++)
  {
    const bool was_null= (*field)->is_null();

    /**
     * assistance for backwards compatibility in production builds.  note: this will not work for ENUM columns.
     */
    if (was_null)
    {
      (*field)->set_default();
      (*field)->set_notnull();
    }

    switch ((*field)->real_type()) {
      case MYSQL_TYPE_LONG:
      case MYSQL_TYPE_FLOAT:
      case MYSQL_TYPE_TIMESTAMP:
        buffer.append("    ");
        buffer.length(buffer.length()-4);
        tmp_int = (*field)->val_int();
        buffer.qs_append((char*)&tmp_int, 4);
        break;
      case MYSQL_TYPE_LONGLONG:
      case MYSQL_TYPE_DOUBLE:
      case MYSQL_TYPE_DATETIME:
        buffer.append("        ");
        buffer.length(buffer.length()-8);
        tmp_long = (*field)->val_int();
        buffer.qs_append((char *)&tmp_long, 8);
        break;
      case MYSQL_TYPE_VARCHAR:
      case MYSQL_TYPE_TINY_BLOB:
      case MYSQL_TYPE_MEDIUM_BLOB:
      case MYSQL_TYPE_LONG_BLOB:
      case MYSQL_TYPE_BLOB:
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_STRING:
        (*field)->val_str(&attribute,&attribute);
        buffer.append(attribute);
        i = attribute.length();
        while (i++ < (*field)->pack_length()) {
          buffer.append(' ');
        }
        break;
      default :
        goto end;
    }

    if (was_null)
      (*field)->set_null();
  }

  i = buffer.length();
  while (i++ < table->s->reclength) {
    buffer.append(' ');
  }

end:
  dbug_tmp_restore_column_map(table->read_set, org_bitmap);
  return 0;
}

int ha_spartan::find_current_row(uchar *buf) {

  my_off_t curr_offset = 0;
  my_bitmap_map *org_bitmap;
  int error, ret;
  bool read_all;
  DBUG_ENTER("ha_tina::find_current_row");

  free_root(&blobroot, MYF(0));

  /*
     We do not read further then local_saved_data_file_length in order
     not to conflict with undergoing concurrent insert.
     */
  ret = read_one_record();
  if (ret == 0) {
    DBUG_RETURN(HA_ERR_END_OF_FILE);
  } else if (ret < (long)io_size) {
    DBUG_RETURN(HA_ERR_INTERNAL_ERROR);
  }

  /* We must read all columns in case a table is opened for update */
  read_all= !bitmap_is_clear_all(table->write_set);
  /* Avoid asserts in ::store() for columns that are not going to be updated */
  org_bitmap= dbug_tmp_use_all_columns(table, table->write_set);
  error= HA_ERR_CRASHED_ON_USAGE;

  memset(buf, 0, table->s->null_bytes);


  for (Field **field=table->field ; *field ; field++)
  {

    buffer.length(0);
    if (curr_offset >= io_size)
      goto err;

    if (read_all || bitmap_is_set(table->read_set, (*field)->field_index)) {

      switch ((*field)->real_type()) {
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_TIMESTAMP:
          if ((*field)->store(uint4korr(io_buf+curr_offset))) {
            goto err;
          }
          break;
        case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_DATETIME:
          if ((*field)->store(uint8korr(io_buf+curr_offset))) {
            goto err;
          }
          break;
        case MYSQL_TYPE_VARCHAR:
        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
        case MYSQL_TYPE_BLOB:
        case MYSQL_TYPE_VAR_STRING:
        case MYSQL_TYPE_STRING:
          if ((*field)->store((const char *)(io_buf+curr_offset), (*field)->pack_length(), buffer.charset(), CHECK_FIELD_WARN))
            goto err;
          if ((*field)->flags & BLOB_FLAG)
          {
            Field_blob *blob= *(Field_blob**) field;
            uchar *src, *tgt;
            uint length, packlength;

            packlength= blob->pack_length_no_ptr();
            length= blob->get_length(blob->ptr);
            memcpy(&src, blob->ptr + packlength, sizeof(char*));
            if (src)
            {
              tgt= (uchar*) alloc_root(&blobroot, length);
              bmove(tgt, src, length);
              memcpy(blob->ptr + packlength, &tgt, sizeof(char*));
            }
          }
          break;
        default :
          goto err;
      }
    }
    curr_offset += (*field)->pack_length();
  }
  error= 0;

err:
  dbug_tmp_restore_column_map(table->write_set, org_bitmap);

  DBUG_RETURN(error);
}

struct st_mysql_storage_engine spartan_storage_engine=
{ MYSQL_HANDLERTON_INTERFACE_VERSION };

static ulong srv_enum_var= 0;
static ulong srv_ulong_var= 0;
static double srv_double_var= 0;

const char *enum_var_names[]=
{
  "e1", "e2", NullS
};

TYPELIB enum_var_typelib=
{
  array_elements(enum_var_names) - 1, "enum_var_typelib",
  enum_var_names, NULL
};

static MYSQL_SYSVAR_ENUM(
    enum_var,                       // name
    srv_enum_var,                   // varname
    PLUGIN_VAR_RQCMDARG,            // opt
    "Sample ENUM system variable.", // comment
    NULL,                           // check
    NULL,                           // update
    0,                              // def
    &enum_var_typelib);             // typelib

static MYSQL_SYSVAR_ULONG(
    ulong_var,
    srv_ulong_var,
    PLUGIN_VAR_RQCMDARG,
    "0..1000",
    NULL,
    NULL,
    8,
    0,
    1000,
    0);

static MYSQL_SYSVAR_DOUBLE(
    double_var,
    srv_double_var,
    PLUGIN_VAR_RQCMDARG,
    "0.500000..1000.500000",
    NULL,
    NULL,
    8.5,
    0.5,
    1000.5,
    0);                             // reserved always 0

static MYSQL_THDVAR_DOUBLE(
    double_thdvar,
    PLUGIN_VAR_RQCMDARG,
    "0.500000..1000.500000",
    NULL,
    NULL,
    8.5,
    0.5,
    1000.5,
    0);

static struct st_mysql_sys_var* spartan_system_variables[]= {
  MYSQL_SYSVAR(enum_var),
  MYSQL_SYSVAR(ulong_var),
  MYSQL_SYSVAR(double_var),
  MYSQL_SYSVAR(double_thdvar),
  NULL
};

// this is an spartan of SHOW_FUNC and of my_snprintf() service
static int show_func_spartan(MYSQL_THD thd, struct st_mysql_show_var *var,
    char *buf)
{
  var->type= SHOW_CHAR;
  var->value= buf; // it's of SHOW_VAR_FUNC_BUFF_SIZE bytes
  my_snprintf(buf, SHOW_VAR_FUNC_BUFF_SIZE,
      "enum_var is %lu, ulong_var is %lu, "
      "double_var is %f, %.6b", // %b is a MySQL extension
      srv_enum_var, srv_ulong_var, srv_double_var, "really");
  return 0;
}

static struct st_mysql_show_var func_status[]=
{
  {"spartan_func_spartan",  (char *)show_func_spartan, SHOW_FUNC},
  {0,0,SHOW_UNDEF}
};

mysql_declare_plugin(spartan)
{
  MYSQL_STORAGE_ENGINE_PLUGIN,
    &spartan_storage_engine,
    "SPARTAN",
    "Brian Aker, MySQL AB",
    "Spartan storage engine",
    PLUGIN_LICENSE_GPL,
    spartan_init_func,                            /* Plugin Init */
    NULL,                                         /* Plugin Deinit */
    0x0001 /* 0.1 */,
    func_status,                                  /* status variables */
    spartan_system_variables,                     /* system variables */
    NULL,                                         /* config options */
    0,                                            /* flags */
}
mysql_declare_plugin_end;
