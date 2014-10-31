mysql
=====

<h3>MySQL-Branch</h3>
在MySQL5.6上添加一个自定义的存储引擎Spartan<br />

<b>特点：</b><br />
<ol>
<li>数据记录定长</li>
<li>数据文件无空洞，删除记录会导致数据在文件内移动</li>
</ol>

<b>目前已有的功能：</b> <br />
<ol>
<li>支持创建/删除表</li>
<li>支持对表的增删改查</li>
</ol>

<b>限制：</b>
<ol>
<li>数据记录长度大小不超过8192</li>
<li>不支持索引（开发中）</li>
<li>不支持事务</li>
</ol>

<h3>InnoDB 学习</h3>
在项目根目录下有一个/learn目录，里面存放了可独立编译的InnoDB的源码库，可用于学习和测试InnoDB的数据结构和代码。<br />
/learn/ut.cc 测试内存分配<br />
/learn/hash.cc 测试InnoDB的Hash表实现<br />
