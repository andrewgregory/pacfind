pacfind - a more powerful pacman search tool
============================================

**NOTE:** This software is still very alpha and is prone to frequent segfaults
and memory leaks.  That being said, it is already capable of fairly advanced
queries.

Usage
+++++

Find libreoffice packages but exclude language packs::

    pacfind -Q -- -name -re libreoffice -not -desc -re lang

Find broken packages::

    pacfind -Q -- -packager allan

Find packages that depend on perl::

    pacfind -Q -- -depends.name perl

Pacman style searching::

    pacfind -Q pacman mirrorlist

License
-------

pacfind is distributed under the terms of the MIT license.  See COPYING for
details.

TODO
----

+ Add ``--format`` option
+ Complete feature parity with ``pacman -Qs`` and ``pacman -Ss``
+ Pacman style output
+ Add support for list fields and operators: ``-depends.name pacman``
+ Allow human readable dates and sizes for values ``-isize -gt 50MB``
+ Fix the multitude of segfaults and memory leaks
+ ``-requiredby`` and ``-satisfies`` fields
