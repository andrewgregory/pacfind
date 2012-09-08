pacfind - a more powerful pacman search tool
============================================

Usage
+++++

``pacfind -Q -- -name -re libreoffice -not -desc -re lang``

License
-------

pacfind is distributed under the terms of the MIT license.  See COPYING for
details.

TODO
----

+ Complete feature parity with ``pacman -Qs`` and ``pacman -Ss``
+ Pacman style output
+ Add support for list fields and operators: ``-depends.name pacman``
+ Allow human readable dates and sizes for values ``-isize -gt 50MB``
+ Fix the multitude of segfaults and memory leaks
