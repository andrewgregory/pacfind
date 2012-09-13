pacfind - a more powerful pacman search tool
============================================

**NOTE:** This software is still very alpha and is prone to frequent segfaults
and memory leaks.  That being said, it is already capable of fairly advanced
queries.

Usage
-----

    pacfind [-QSqs] [--] [OP] [FIELD] [CMP] value [JOIN] ...

Options
*******

-Q
    Search installed packages.  May be combined with ``-S``.

-S
    Search sync packages.  May be combined with ``-Q``.

-q
    Only print the names of matching packages.

-s
    No-op, provided for compatibility with pacman.

Query Syntax
************

Operators
+++++++++

+ -not
+ -go - group queries
+ -gc - close query group

Fields
++++++

If no field is specified name and description will be searched.

String Scalars
^^^^^^^^^^^^^^

+ -name
+ -desc
+ -filename
+ -url
+ -packager
+ -version
+ -md5sum
+ -sha256sum

Integer Scalars
^^^^^^^^^^^^^^^

+ -isize
+ -size
+ -builddate
+ -installdate

Package Lists
^^^^^^^^^^^^^

Package list fields may be used alone or with a sub-selector separated by
a period.  When used alone the package field will be searched directly for the
term.  When used with a sub-selector, the field will be expanded to the
packages it includes which will then be searched normally.  For example,
``pacfind -Q -- -depends bash`` will find only those packages which explicitly
depend on ``bash`` whereas ``pacfind -Q -- -depends.name bash`` will also find
packages whose dependency on ``sh`` is satisfied by ``bash``.  In the second
form, these may be followed by '%' to search recursively.

+ -depends
+ -optdepends
+ -conflicts
+ -provides
+ -replaces
+ -requiredby - pacfind will **NOT** limit this to local packages; you probably
  only want to use this in conjunction with ``-Q``.

Comparison Operators
++++++++++++++++++++

If no comparison operator is provided ``-re`` will be used for string fields
and ``-eq`` for numeric fields.

+ -eq
+ -ne
+ -gt
+ -ge
+ -lt
+ -le
+ -re
+ -nr

Join Operators
++++++++++++++

If no join operator is provided, ``-and`` will be used.

+ -and
+ -or
+ -xor

Examples
********

Find libreoffice packages but exclude language packs::

    pacfind -Q -- -name -re libreoffice -not -desc -re lang

Find broken packages::

    pacfind -Q -- -packager allan

Find packages that depend on a package that depends on perl::

    pacfind -Q -- -depends.depends.name perl

Pacman style searching::

    pacfind -Q pacman mirrorlist

Search only explicitly installed packages::

    pacman -Qqe | pacfind -- -desc perl

Search for packages with perl anywhere in their dependency chains::

    pacfind -- -depends%.name perl

License
-------

pacfind is distributed under the terms of the MIT license.  See COPYING for
details.

TODO
----

+ Add ``--format`` option
+ Complete feature parity with ``pacman -Qs`` and ``pacman -Ss``
+ Pacman style output
+ Allow human readable dates and sizes for values ``-isize -gt 50MB``
+ List field counts
+ Fix the multitude of segfaults and memory leaks
+ Optimize node resolution order
+ Threads
+ Remaining Fields:

  - satisifes
  - licenses
  - arch
  - groups
  - script
  - installreason
