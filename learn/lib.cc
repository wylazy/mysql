#ifndef __MY_INNODB_VARIABLES_DEF_
#define __MY_INNODB_VARIABLES_DEF_

#include <debug_sync.h> // DEBUG_SYNC

#include <univ.i>
#include "buf0dump.h"
#include "buf0lru.h"
#include "buf0flu.h"
#include "buf0dblwr.h"
#include "btr0sea.h"
#include "os0file.h"
#include "os0thread.h"
#include "srv0start.h"
#include "srv0srv.h"
#include "trx0roll.h"
#include "trx0trx.h"

#include "trx0sys.h"
#include "mtr0mtr.h"
#include "rem0types.h"
#include "row0ins.h"
#include "row0mysql.h"
#include "row0sel.h"
#include "row0upd.h"
#include "log0log.h"
#include "lock0lock.h"
#include "dict0crea.h"
#include "btr0cur.h"
#include "btr0btr.h"
#include "fsp0fsp.h"
#include "sync0sync.h"
#include "fil0fil.h"
#include "trx0xa.h"
#include "row0merge.h"
#include "dict0boot.h"
#include "dict0stats.h"
#include "dict0stats_bg.h"
#include "ha_prototypes.h"
#include "ut0mem.h"
#include "ibuf0ibuf.h"
#include "dict0dict.h"
#include "srv0mon.h"
#include "api0api.h"
#include "api0misc.h"
#include "pars0pars.h"
#include "fts0fts.h"
#include "fts0types.h"
#include "row0import.h"
#include "row0quiesce.h"
#ifdef UNIV_DEBUG
#include "trx0purge.h"
#endif /* UNIV_DEBUG */
#include "fts0priv.h"
#include "page0zip.h"

#include <handler0alter.h>

ulong max_connections;
uint opt_debug_sync_timeout = 10;
CHARSET_INFO *system_charset_info= &my_charset_utf8_general_ci;;

/******************************************************************//**
Returns true if the thread supports XA,
global value of innodb_supports_xa if thd is NULL.
@return	true if thd has XA support */
UNIV_INTERN
ibool
thd_supports_xa(
/*============*/
	THD*	thd)	/*!< in: thread handle, or NULL to query
			the global innodb_supports_xa */
{
  return true;
}

UNIV_INTERN
void
ib_logf(
/*====*/
	ib_log_level_t	level,		/*!< in: warning level */
	const char*	format,		/*!< printf format */
	...)				/*!< Args */
{
  /*NULL*/
}

/******************************************************************//**
Returns true if the transaction this thread is processing has edited
non-transactional tables. Used by the deadlock detector when deciding
which transaction to rollback in case of a deadlock - we try to avoid
rolling back transactions that have edited non-transactional tables.
@return	true if non-transactional tables have been edited */
UNIV_INTERN
ibool
thd_has_edited_nontrans_tables(
/*===========================*/
	THD*	thd)	/*!< in: thread handle */
{
	 return false;
}

/******************************************************************//**
Check if the transaction is an auto-commit transaction. TRUE also
implies that it is a SELECT (read-only) transaction.
@return	true if the transaction is an auto commit read-only transaction. */
UNIV_INTERN
ibool
thd_trx_is_auto_commit(
/*===================*/
	THD*	thd)	/*!< in: thread handle, can be NULL */
{
  return true;
}


/******************************************************************//**
Returns true if transaction should be flagged as read-only.
@return	true if the thd is marked as read-only */
UNIV_INTERN
ibool
thd_trx_is_read_only(
/*=================*/
	THD*	thd)	/*!< in: thread handle */
{
  return false;
}


/******************************************************************//**
Gets information on the durability property requested by thread.
Used when writing either a prepare or commit record to the log
buffer. @return the durability property. */
UNIV_INTERN
enum durability_properties
thd_requested_durability(
/*=====================*/
	const THD* thd)	/*!< in: thread handle */
{
  return HA_REGULAR_DURABILITY;
}

/*************************************************************//**
Prints info of a THD object (== user session thread) to the given file. */
UNIV_INTERN
void
innobase_mysql_print_thd(
/*=====================*/
	FILE*	f,		/*!< in: output stream */
	THD*	thd,		/*!< in: MySQL THD object */
	uint	max_query_len)	/*!< in: max query length to print, or 0 to
				use the default max length */
{
}

/******************************************************************//**
Get the error message format string.
@return the format string or 0 if not found. */
UNIV_INTERN
const char*
innobase_get_err_msg(
/*=================*/
	int	error_code)	/*!< in: MySQL error code */
{
	return ("hello error");
}

/******************************************************************//**
Get the variable length bounds of the given character set. */
UNIV_INTERN
void
innobase_get_cset_width(
/*====================*/
	ulint	cset,		/*!< in: MySQL charset-collation code */
	ulint*	mbminlen,	/*!< out: minimum length of a char (in bytes) */
	ulint*	mbmaxlen)	/*!< out: maximum length of a char (in bytes) */
{
  *mbminlen = 1;
  *mbmaxlen = 3;
}

/**********************************************************************//**
Determines the connection character set.
@return	connection character set */
UNIV_INTERN
struct charset_info_st*
innobase_get_charset(
/*=================*/
	THD*	mysql_thd)	/*!< in: MySQL thread handle */
{
  return NULL;
}

/**********************************************************************//**
Determines the current SQL statement.
@return	SQL statement string */
UNIV_INTERN
const char*
innobase_get_stmt(
/*==============*/
	THD*	thd,		/*!< in: MySQL thread handle */
	size_t*	length)		/*!< out: length of the SQL statement */
{
  *length = 3;
  return "sql";
}

/******************************************************************//**
Strip dir name from a full path name and return only the file name
@return file name or "null" if no file name */
UNIV_INTERN
const char*
innobase_basename(
/*==============*/
	const char*	path_name)	/*!< in: full path name */
{
	const char*	name = base_name(path_name);

	return((name) ? name : "null");
}

/******************************************************************//**
Makes all characters in a NUL-terminated UTF-8 string lower case. */
UNIV_INTERN
void
innobase_casedn_str(
/*================*/
	char*	a)	/*!< in/out: string to put in lower case */
{
}

/**********************************************************************//**
Get the current setting of the table_def_size global parameter. We do
a dirty read because for one there is no synchronization object and
secondly there is little harm in doing so even if we get a torn read.
@return	value of table_def_size */
UNIV_INTERN
ulint
innobase_get_table_cache_size(void)
/*===============================*/
{
	return(1024*1024);
}


/******************************************************************//**
Returns true if the thread is the replication thread on the slave
server. Used in srv_conc_enter_innodb() to determine if the thread
should be allowed to enter InnoDB - the replication thread is treated
differently than other threads. Also used in
srv_conc_force_exit_innodb().
@return	true if thd is the replication thread */
UNIV_INTERN
ibool
thd_is_replication_slave_thread(
/*============================*/
	THD*	thd)	/*!< in: thread handle */
{
	return false;
}

/******************************************************************//**
Compares NUL-terminated UTF-8 strings case insensitively.
@return	0 if a=b, <0 if a<b, >1 if a>b */
UNIV_INTERN
int
innobase_strcasecmp(
/*================*/
	const char*	a,	/*!< in: first string to compare */
	const char*	b)	/*!< in: second string to compare */
{
  return strcasecmp(a, b);
}

/*****************************************************************//**
Frees a possible InnoDB trx object associated with the current THD.
@return	0 or error number */
UNIV_INTERN
int
innobase_close_thd(
/*===============*/
	THD*		thd)	/*!< in: handle to the MySQL thread of the user
				whose resources should be free'd */
{
  return 0;
}

/**********************************************************************//**
Determines if the currently running transaction has been interrupted.
@return	TRUE if interrupted */
UNIV_INTERN
ibool
trx_is_interrupted(
/*===============*/
	const trx_t*	trx)	/*!< in: transaction */
{
	return false;
}

void thd_wait_begin(MYSQL_THD thd, int wait_type) {
  
}

void thd_wait_end(MYSQL_THD thd, int wait_type) {
  
}

void thd_wait_end(MYSQL_THD thd) {
  
}

void thd_wait_begin(MYSQL_THD thd) {
  
}

/******************************************************************//**
This function is used to find the storage length in bytes of the first n
characters for prefix indexes using a multibyte character set. The function
finds charset information and returns length of prefix_len characters in the
index field in bytes.
@return	number of bytes occupied by the first n characters */
UNIV_INTERN
ulint
innobase_get_at_most_n_mbchars(
/*===========================*/
	ulint charset_id,	/*!< in: character set id */
	ulint prefix_len,	/*!< in: prefix length in bytes of the index
				(this has to be divided by mbmaxlen to get the
				number of CHARACTERS n in the prefix) */
	ulint data_len,		/*!< in: length of the string in bytes */
	const char* str)	/*!< in: character string */
{
  return 0;
}

UNIV_INTERN
ibool
trx_is_strict(
/*==========*/
	trx_t*	trx)	/*!< in: transaction */
{
  return false;
}

/**********************************************************************
Converts an identifier from my_charset_filename to UTF-8 charset.
@return result string length, as returned by strconvert() */
uint
innobase_convert_to_filename_charset(
/*=================================*/
	char*		to,	/* out: converted identifier */
	const char*	from,	/* in: identifier to convert */
	ulint		len)	/* in: length of 'to', in bytes */
{
  to = (char *)from;
  return len;
}
/**********************************************************************
Converts an identifier from my_charset_filename to UTF-8 charset.
@return result string length, as returned by strconvert() */
uint
innobase_convert_to_system_charset(
/*===============================*/
	char*		to,	/* out: converted identifier */
	const char*	from,	/* in: identifier to convert */
	ulint		len,	/* in: length of 'to', in bytes */
	uint*		errors)	/* out: error return */
{
  to = (char *)from;
  return len;
}

/******************************************************************//**
Converts an identifier to UTF-8. */
UNIV_INTERN
void
innobase_convert_from_id(
/*=====================*/
	struct charset_info_st*	cs,	/*!< in: the 'from' character set */
	char*			to,	/*!< out: converted identifier */
	const char*		from,	/*!< in: identifier to convert */
	ulint			len)	/*!< in: length of 'to', in bytes */
{
  to = (char *)from;
}

/******************************************************************//**
Converts an identifier to a table name. */
UNIV_INTERN
void
innobase_convert_from_table_id(
/*===========================*/
	struct charset_info_st*	cs,	/*!< in: the 'from' character set */
	char*			to,	/*!< out: converted identifier */
	const char*		from,	/*!< in: identifier to convert */
	ulint			len)	/*!< in: length of 'to', in bytes */
{
  to = (char *)from;
}

unsigned long thd_get_thread_id(const MYSQL_THD thd) {
  return 0;
}

/**********************************************************************//**
Get the current setting of the lower_case_table_names global parameter from
mysqld.cc. We do a dirty read because for one there is no synchronization
object and secondly there is little harm in doing so even if we get a torn
read.
@return	value of lower_case_table_names */
UNIV_INTERN
ulint
innobase_get_lower_case_table_names(void)
{
	return 0;
}

/******************************************************************//**
Use this when the args are passed to the format string from
errmsg-utf8.txt directly as is.

Push a warning message to the client, it is a wrapper around:

void push_warning_printf(
	THD *thd, Sql_condition::enum_warning_level level,
	uint code, const char *format, ...);
*/
UNIV_INTERN
void
ib_senderrf(
/*========*/
	THD*		thd,		/*!< in/out: session */
	ib_log_level_t	level,		/*!< in: warning level */
	ib_uint32_t	code,		/*!< MySQL error code */
	...)				/*!< Args */
{
}
/*****************************************************************//**
A wrapper function of innobase_convert_name(), convert a table or
index name to the MySQL system_charset_info (UTF-8) and quote it if needed.
@return	pointer to the end of buf */
UNIV_INTERN
void
innobase_format_name(
/*==================*/
	char*		buf,	/*!< out: buffer for converted identifier */
	ulint		buflen,	/*!< in: length of buf, in bytes */
	const char*	name,	/*!< in: index or table name to format */
	ibool		is_index_name) /*!< in: index name */
{
  *buf = 0;
}

UNIV_INTERN
void
ib_errf(
/*====*/
	THD*		thd,		/*!< in/out: session */
	ib_log_level_t	level,		/*!< in: warning level */
	ib_uint32_t	code,		/*!< MySQL error code */
	const char*	format,		/*!< printf format */
	...)				/*!< Args */
{
}

/******************************************************************//**
Returns the NUL terminated value of glob_hostname.
@return	pointer to glob_hostname. */
UNIV_INTERN
const char*
server_get_hostname()
/*=================*/
{
	return "localhost";
}
/******************************************************************//**
Returns true if the thread is executing a SELECT statement.
@return	true if thd is executing SELECT */
UNIV_INTERN
ibool
thd_is_select(
/*==========*/
	const THD*	thd)	/*!< in: thread handle */
{
	return false;
}

/**
 * Index Condition Pushdown interface implementation
 */

/*************************************************************//**
InnoDB index push-down condition check
@return ICP_NO_MATCH, ICP_MATCH, or ICP_OUT_OF_RANGE */
UNIV_INTERN
enum icp_result
innobase_index_cond(
/*================*/
	void*	file)	/*!< in/out: pointer to ha_innobase */
{
	DBUG_ENTER("innobase_index_cond");
	DBUG_RETURN(ICP_NO_MATCH);
}

/*******************************************************************//**
Formats the raw data in "data" (in InnoDB on-disk format) that is of
type DATA_(CHAR|VARCHAR|MYSQL|VARMYSQL) using "charset_coll" and writes
the result to "buf". The result is converted to "system_charset_info".
Not more than "buf_size" bytes are written to "buf".
The result is always NUL-terminated (provided buf_size > 0) and the
number of bytes that were written to "buf" is returned (including the
terminating NUL).
@return	number of bytes that were written */
UNIV_INTERN
ulint
innobase_raw_format(
/*================*/
	const char*	data,		/*!< in: raw data */
	ulint		data_len,	/*!< in: raw data length
					in bytes */
	ulint		charset_coll,	/*!< in: charset collation */
	char*		buf,		/*!< out: output buffer */
	ulint		buf_size)	/*!< in: output buffer size
					in bytes */
{
  return 0;
}
/*************************************************************//**
Copies an InnoDB row to table->record[0]. */
UNIV_INTERN
void
innobase_row_to_mysql(
/*==================*/
  struct TABLE*   table,  /*!< in/out: MySQL table */
  const dict_table_t* itab, /*!< in: InnoDB table */
  const dtuple_t*   row)  /*!< in: InnoDB row */
{
}

/*********************************************************************//**
Creates a temporary file.
@return	temporary file descriptor, or < 0 on error */
UNIV_INTERN
int
innobase_mysql_tmpfile(void)
/*========================*/
{
  return 0;
}

/*************************************************************//**
Copies an InnoDB index entry to table->record[0]. */
UNIV_INTERN
void
innobase_fields_to_mysql(
/*=====================*/
	struct TABLE*		table,	/*!< in/out: MySQL table */
	const dict_index_t*	index,	/*!< in: InnoDB index */
	const dfield_t*		fields)	/*!< in: InnoDB index fields */
{
}

/*****************************************************************//**
Invalidates the MySQL query cache for the table. */
UNIV_INTERN
void
innobase_invalidate_query_cache(
/*============================*/
	trx_t*		trx,		/*!< in: transaction which
					modifies the table */
	const char*	full_name,	/*!< in: concatenation of
					database name, null char NUL,
					table name, null char NUL;
					NOTE that in Windows this is
					always in LOWER CASE! */
	ulint		full_name_len)	/*!< in: full name length where
					also the null chars count */
{
}
/******************************************************************//**
compare two character string according to their charset. */
UNIV_INTERN
int
innobase_fts_text_cmp(
/*==================*/
	const void*	cs,		/*!< in: Character set */
	const void*     p1,		/*!< in: key */
	const void*     p2)		/*!< in: node */
{
  return 0;
}
/*************************************************************//**
Resets table->record[0]. */
UNIV_INTERN
void
innobase_rec_reset(
/*===============*/
	TABLE*			table)		/*!< in/out: MySQL table */
{
}
/**********************************************************************
Check if the length of the identifier exceeds the maximum allowed.
return true when length of identifier is too long. */
UNIV_INTERN
my_bool
innobase_check_identifier_length(
/*=============================*/
	const char*	id)	/* in: FK identifier to check excluding the
				database portion. */
{
  return true;
}
/*****************************************************************//**
Convert a table or index name to the MySQL system_charset_info (UTF-8)
and quote it if needed.
@return	pointer to the end of buf */
UNIV_INTERN
char*
innobase_convert_name(
/*==================*/
	char*		buf,	/*!< out: buffer for converted identifier */
	ulint		buflen,	/*!< in: length of buf, in bytes */
	const char*	id,	/*!< in: identifier to convert */
	ulint		idlen,	/*!< in: length of id, in bytes */
	THD*		thd,	/*!< in: MySQL connection thread, or NULL */
	ibool		table_id)/*!< in: TRUE=id is a table or database name;
				FALSE=id is an index name */
{
  return buf;
}

/*
  Converts a string between character sets

  SYNOPSIS
    strconvert()
    from_cs       source character set
    from          source, a null terminated string
    to            destination buffer
    to_length     destination buffer length

  NOTES
    'to' is always terminated with a '\0' character.
    If there is no enough space to convert whole string,
    only prefix is converted, and terminated with '\0'.

  RETURN VALUES
    result string length
*/


uint strconvert(CHARSET_INFO *from_cs, const char *from,
                CHARSET_INFO *to_cs, char *to, uint to_length, uint *errors)
{
  int cnvres;
  my_wc_t wc;
  char *to_start= to;
  uchar *to_end= (uchar*) to + to_length - 1;
  my_charset_conv_mb_wc mb_wc= from_cs->cset->mb_wc;
  my_charset_conv_wc_mb wc_mb= to_cs->cset->wc_mb;
  uint error_count= 0;

  while (1)
  {
    /*
      Using 'from + 10' is safe:
      - it is enough to scan a single character in any character set.
      - if remaining string is shorter than 10, then mb_wc will return
        with error because of unexpected '\0' character.
    */
    if ((cnvres= (*mb_wc)(from_cs, &wc,
                          (uchar*) from, (uchar*) from + 10)) > 0)
    {
      if (!wc)
        break;
      from+= cnvres;
    }
    else if (cnvres == MY_CS_ILSEQ)
    {
      error_count++;
      from++;
      wc= '?';
    }
    else
      break; // Impossible char.

outp:

    if ((cnvres= (*wc_mb)(to_cs, wc, (uchar*) to, to_end)) > 0)
      to+= cnvres;
    else if (cnvres == MY_CS_ILUNI && wc != '?')
    {
      error_count++;
      wc= '?';
      goto outp;
    }
    else
      break;
  }
  *to= '\0';
  *errors= error_count;
  return (uint32) (to - to_start);

}
/******************************************************************//**
Get the first character's code position for FTS index partition. */
UNIV_INTERN
ulint
innobase_strnxfrm(
/*==============*/
	const CHARSET_INFO*
			cs,		/*!< in: Character set */
	const uchar*	str,		/*!< in: string */
	const ulint	len)		/*!< in: string length */
{
  return 0;
}
/*************************************************************//**
Get the next token from the given string and store it in *token. */
UNIV_INTERN
CHARSET_INFO*
innobase_get_fts_charset(
/*=====================*/
	int		mysql_type,	/*!< in: MySQL type */
	uint		charset_number)	/*!< in: number of the charset */
{
  return default_charset_info;
}
/*************************************************************//**
Get the next token from the given string and store it in *token.
It is mostly copied from MyISAM's doc parsing function ft_simple_get_word()
@return length of string processed */
UNIV_INTERN
ulint
innobase_mysql_fts_get_token(
/*=========================*/
	CHARSET_INFO*	cs,		/*!< in: Character set */
	const byte*	start,		/*!< in: start of text */
	const byte*	end,		/*!< in: one character past end of
					text */
	fts_string_t*	token,		/*!< out: token's text */
	ulint*		offset)		/*!< out: offset to token,
					measured as characters from
					'start' */
{
  return 0;
}
/******************************************************************//**
Makes all characters in a string lower case. */
UNIV_INTERN
size_t
innobase_fts_casedn_str(
/*====================*/
	CHARSET_INFO*	cs,	/*!< in: Character set */
	char*		src,	/*!< in: string to put in lower case */
	size_t		src_len,/*!< in: input string length */
	char*		dst,	/*!< in: buffer for result string */
	size_t		dst_len)/*!< in: buffer size */
{
  return 0;
}

/******************************************************************//**
compare two character string according to their charset. */
UNIV_INTERN
int
innobase_fts_text_cmp_prefix(
/*=========================*/
	const void*	cs,		/*!< in: Character set */
	const void*	p1,		/*!< in: prefix key */
	const void*	p2)		/*!< in: value to compare */
{
  return 0;
}
/******************************************************************//**
compare two character string case insensitively according to their charset. */
UNIV_INTERN
int
innobase_fts_text_case_cmp(
/*=======================*/
	const void*	cs,		/*!< in: Character set */
	const void*     p1,		/*!< in: key */
	const void*     p2)		/*!< in: node */
{
  return 0;
}

/******************************************************************//**
Returns the lock wait timeout for the current connection.
@return	the lock wait timeout, in seconds */
UNIV_INTERN
ulong
thd_lock_wait_timeout(
/*==================*/
	THD*	thd)	/*!< in: thread handle, or NULL to query
			the global innodb_lock_wait_timeout */
{
  return 60;
}
/******************************************************************//**
Set the time waited for the lock for the current query. */
UNIV_INTERN
void
thd_set_lock_wait_time(
/*===================*/
	THD*	thd,	/*!< in/out: thread handle */
	ulint	value)	/*!< in: time waited for the lock */
{
}

/*************************************************************//**
InnoDB uses this function to compare two data fields for which the data type
is such that we must use MySQL code to compare them. NOTE that the prototype
of this function is in rem0cmp.cc in InnoDB source code! If you change this
function, remember to update the prototype there!
@return	1, 0, -1, if a is greater, equal, less than b, respectively */
UNIV_INTERN
int
innobase_mysql_cmp(
/*===============*/
	int		mysql_type,	/*!< in: MySQL type */
	uint		charset_number,	/*!< in: number of the charset */
	const unsigned char* a,		/*!< in: data field */
	unsigned int	a_length,	/*!< in: data field length,
					not UNIV_SQL_NULL */
	const unsigned char* b,		/*!< in: data field */
	unsigned int	b_length)	/*!< in: data field length,
					not UNIV_SQL_NULL */
{
  unsigned int m = min(a_length, b_length);
  return strncmp((char *)a, (char *)b, m);
}
/*************************************************************//**
InnoDB uses this function to compare two data fields for which the data type
is such that we must use MySQL code to compare them. NOTE that the prototype
of this function is in rem0cmp.c in InnoDB source code! If you change this
function, remember to update the prototype there!
@return	1, 0, -1, if a is greater, equal, less than b, respectively */
UNIV_INTERN
int
innobase_mysql_cmp_prefix(
/*======================*/
	int		mysql_type,	/*!< in: MySQL type */
	uint		charset_number,	/*!< in: number of the charset */
	const unsigned char* a,		/*!< in: data field */
	unsigned int	a_length,	/*!< in: data field length,
					not UNIV_SQL_NULL */
	const unsigned char* b,		/*!< in: data field */
	unsigned int	b_length)	/*!< in: data field length,
					not UNIV_SQL_NULL */
{
  unsigned int m = min(a_length, b_length);
  return strncmp((char *)a, (char *)b, m);
}

/*************************************************************//**
Copies an InnoDB record to table->record[0]. */
UNIV_INTERN
void
innobase_rec_to_mysql(
/*==================*/
	struct TABLE*		table,	/*!< in/out: MySQL table */
	const rec_t*		rec,	/*!< in: record */
	const dict_index_t*	index,	/*!< in: index */
	const ulint*		offsets)/*!< in: rec_get_offsets(
					rec, index, ...) */
{
}

/*********************************************************************//**
Compute the next autoinc value.

For MySQL replication the autoincrement values can be partitioned among
the nodes. The offset is the start or origin of the autoincrement value
for a particular node. For n nodes the increment will be n and the offset
will be in the interval [1, n]. The formula tries to allocate the next
value for a particular node.

Note: This function is also called with increment set to the number of
values we want to reserve for multi-value inserts e.g.,

	INSERT INTO T VALUES(), (), ();

innobase_next_autoinc() will be called with increment set to 3 where
autoinc_lock_mode != TRADITIONAL because we want to reserve 3 values for
the multi-value INSERT above.
@return	the next value */
UNIV_INTERN
ulonglong
innobase_next_autoinc(
/*==================*/
	ulonglong	current,	/*!< in: Current value */
	ulonglong	need,		/*!< in: count of values needed */
	ulonglong	step,		/*!< in: AUTOINC increment step */
	ulonglong	offset,		/*!< in: AUTOINC offset */
	ulonglong	max_value)	/*!< in: max value for type */
{
	ulonglong	next_value;
	ulonglong	block = need * step;

	/* Should never be 0. */
	ut_a(need > 0);
	ut_a(block > 0);
	ut_a(max_value > 0);

	/* According to MySQL documentation, if the offset is greater than
	the step then the offset is ignored. */
	if (offset > block) {
		offset = 0;
	}

	/* Check for overflow. Current can be > max_value if the value is
	in reality a negative value.The visual studio compilers converts
	large double values automatically into unsigned long long datatype
	maximum value */

	if (block >= max_value
	    || offset > max_value
	    || current >= max_value
	    || max_value - offset <= offset) {

		next_value = max_value;
	} else {
		ut_a(max_value > current);

		ulonglong	free = max_value - current;

		if (free < offset || free - offset <= block) {
			next_value = max_value;
		} else {
			next_value = 0;
		}
	}

	if (next_value == 0) {
		ulonglong	next;

		if (current > offset) {
			next = (current - offset) / step;
		} else {
			next = (offset - current) / step;
		}

		ut_a(max_value > next);
		next_value = next * step;
		/* Check for multiplication overflow. */
		ut_a(next_value >= next);
		ut_a(max_value > next_value);

		/* Check for overflow */
		if (max_value - next_value >= block) {

			next_value += block;

			if (max_value - next_value >= offset) {
				next_value += offset;
			} else {
				next_value = max_value;
			}
		} else {
			next_value = max_value;
		}
	}

	ut_a(next_value != 0);
	ut_a(next_value <= max_value);

	return(next_value);
}

/**
Postfix increment
@return the next value to insert */
UNIV_INTERN
ulonglong
ib_sequence_t::operator++(int) UNIV_NOTHROW
{
	ulonglong	current = m_next_value;

	ut_ad(!m_eof);
	ut_ad(m_max_value > 0);

	m_next_value = innobase_next_autoinc(
		current, 1, m_increment, m_offset, m_max_value);

	if (m_next_value == m_max_value && current == m_next_value) {
		m_eof = true;
	}

	return(current);
}

/**
 *   Execute requested action at a synchronization point.
 *
 *   @param[in]     thd                thread handle
 *   @param[in]     sync_point_name    name of synchronization point
 *   @param[in]     name_len           length of sync point name
 **/

void debug_sync(THD *thd, const char *sync_point_name, size_t name_len)
{
  return;

}
#endif
