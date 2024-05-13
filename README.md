```
███ ███ ██  ██  ███ ███ █   ██     █ █ ███ █ █   ███
█   █ █ █ █ █ █ █   █   █   █ █    █ █  █  █ █   █
██  ███ ██  ███ ██  ██  █   █ █ ██ █ █  █  █ █   ███
█   █ █ █ █ █ █ █   █   █   █ █    █ █  █  █ █     █
█   █ █ █ █ ██  █   ███ ███ ██     ███  █  █ ███ ███
```

farbfeld is a lossless image format which is easy to parse, pipe and compress.
It has the following format:

    ╔════════╤═════════════════════════════════════════════════════════╗
    ║ Bytes  │ Description                                             ║
    ╠════════╪═════════════════════════════════════════════════════════╣
    ║ 8      │ "farbfeld" magic value                                  ║
    ╟────────┼─────────────────────────────────────────────────────────╢
    ║ 4      │ 32-Bit BE unsigned integer (width)                      ║
    ╟────────┼─────────────────────────────────────────────────────────╢
    ║ 4      │ 32-Bit BE unsigned integer (height)                     ║
    ╟────────┼─────────────────────────────────────────────────────────╢
    ║ [2222] │ 4x16-Bit BE unsigned integers [RGBA] / pixel, row-major ║
    ╚════════╧═════════════════════════════════════════════════════════╝

Refer to https://tools.suckless.org/farbfeld/ for general information and example implementations
utilising farbfeld.

----

Farbfeld is an underrated and overlooked image format that has quite a range of tools, filters and
generators that can be used manually or programmatically.

The use of such programs can, unfortunately, be quite difficult at times due to lack of information
as there is typically no help output or man pages to refer to. Reading cryptic C code is also not
something most people are fluent in.

The aim of this project is to add man pages and basic help output for the programs in the
Farbfeld Utilities collection at
[http://zzo38computer.org](http://zzo38computer.org/fossil/farbfeld.ui/wiki?name=Documentation).

There is documentation on the website and this project complements that.

At a bare minimum the man pages should give the user a rough understanding of what the program is
for and to give an example of how it is used.

An example man page from this project:

```
FF-ICOSA(1)                            General Commands Manual                          FF-ICOSA(1)

NAME
       ff-icosa — psychedelic icosahedral fractal picture generator

SYNOPSIS
       ff-icosa <width> <height> <scale amount> <?seed?> <?option?>

DESCRIPTION
       ff-icosa  generates psychedelic icosahedral fractal pictures in farbfeld(5) image format and
       writes the result to stdout.

       This program does not read from standard in.

       In case of an error ff-icosa writes a diagnostic message to stderr.

EXIT STATUS
       0       Image processed successfully.

       1       An error occurred.

OPTIONS
       The program takes up to five arguments:
          - width
          - height
          - scale amount (not necessarily an integer)
          - random seed (optional)
          - option (integer from 0 to 127)

       The option defaults to 0 and has various effects.

EXAMPLES
       $ ff-icosa 640 480 68.83 1.49 > image-icosa.ff

       $ ff-icosa 640 480 281.83 41.49 89 > image-icosa.ff

SEE ALSO
       farbfeld(5), farbfeld-utils(7)

farbfeld-utils                               2024-04-03                                 FF-ICOSA(1)
```

## Installation

It should be noted that the Makefile will only install (and uninstall) the man pages - it will not
compile the programs themselves.

```sh
$ sudo make install
```

All the program `.c` files has a header that contains the compilation command for that program,
which means that they can be run as shell scripts to compile them, e.g.

```sh
$ sh ff-hjoin.c
```

The compiled binaries will be placed under the `~/bin/` directory.

To compile all programs run:

```sh
$ for FILE in ff-*.c ff2*.c *2ff.c; do sh $FILE; done
```

Some of the programs may have depencencies on additional libraries; for example `ff-sql` which
depends on `sqlite3.o` being present.

It should be noted that conversion tools like `avsff` and `ffavs` have been renamed to
`avs2ff` and `ff2avs` respectively to be in line with the naming scheme of the suckless project
and to avoid confusion with other tools such as `groff`, `troff` or `ffmpeg`.

----

References:
   - https://tools.suckless.org/farbfeld/
   - http://justsolve.archiveteam.org/wiki/Farbfeld_Utilities
   - http://zzo38computer.org/fossil/farbfeld.ui/home
   - http://zzo38computer.org/fossil/farbfeld.ui/wiki?name=Documentation
   - BSB
      - http://justsolve.archiveteam.org/wiki/BSB
      - https://libbsb.sourceforge.net/bsb_file_format.html
      - https://opencpn.org/wiki/dokuwiki/doku.php?id=opencpn:supplementary_software:chart_conversion_manual:bsb_kap_file_format
   - CGA
      - https://minuszerodegrees.net/oa/OA%20-%20IBM%20Color%20Graphics%20Monitor%20Adapter%20(CGA).pdf
      - https://nerdlypleasures.blogspot.com/2016/05/ibms-cga-hardware-explained.html
      - https://en.wikipedia.org/wiki/Color_Graphics_Adapter
      - https://www.seasip.info/VintagePC/cga.html
   - CUT
      - http://justsolve.archiveteam.org/wiki/Dr._Halo_CUT
   - DRCS
      - https://terminals-wiki.org/wiki/index.php/DEC_VT320
      - https://en.wikipedia.org/wiki/VT320
      - https://www.vt100.net/dec/vt320/soft_characters
   - Fuun RNA
      - https://esolangs.org/wiki/Fuun_RNA
      - https://save-endo.cs.uu.nl/
      - https://save-endo.cs.uu.nl/Endo.pdf
   - GIF
      - http://justsolve.archiveteam.org/wiki/GIF
   - GLE
      - http://justsolve.archiveteam.org/wiki/GLE
      - http://fileformats.archiveteam.org/wiki/Award_BIOS_logo
      - https://en.wikipedia.org/wiki/Color_Graphics_Adapter#Color_palette
   - JEF
      - http://www.grandembroiderydesigns.com/help/janomeembroideryformat.php
      - https://www.janome.com/janome-embroidery-formats/
   - MacPaint
      - http://justsolve.archiveteam.org/wiki/MacPaint
      - http://justsolve.archiveteam.org/wiki/MacBinary
      - http://justsolve.archiveteam.org/wiki/PackBits
   - Maki-chan / MAKI
      - https://mooncore.eu/bunny/txt/makichan.htm
   - MIFF
      - http://justsolve.archiveteam.org/wiki/MIFF
   - MPP
      - https://github.com/zerkman/mpp
      - http://zzo38computer.org/fossil/farbfeld.ui/wiki?name=ffmpp
      - http://justsolve.archiveteam.org/wiki/Multi_Palette_Picture
   - MRF
      - http://justsolve.archiveteam.org/wiki/MRF_(Monochrome_Recursive_Format)
      - https://netpbm.sourceforge.net/doc/mrf.html
   - MZM
      - https://www.digitalmzx.com/
      - http://justsolve.archiveteam.org/wiki/MZM
   - NEO
      - http://justsolve.archiveteam.org/wiki/NEOchrome
      - https://wiki.multimedia.cx/index.php?title=Neochrome
      - https://www.fileformat.info/format/atari/egff.htm
   - PBM
      - http://justsolve.archiveteam.org/wiki/Netpbm_formats
      - http://justsolve.archiveteam.org/wiki/Portable_Arbitrary_Map
      - https://netpbm.sourceforge.net/doc/pam.html
   - PMART
      - http://justsolve.archiveteam.org/wiki/Palette_Master
   - Psycopathicteen Tile Compressor
      - http://zzo38computer.org/fossil/farbfeld.ui/wiki?name=ffpsytc
      - http://forums.nesdev.com/viewtopic.php?p=206353#p206353
   - QOI
      - http://justsolve.archiveteam.org/wiki/Quite_OK_Image_Format
      - https://qoiformat.org/qoi-specification.pdf
      - https://github.com/phoboslab/qoi/blob/master/qoi.h
      - https://gitlab.com/dan9er/farbfeld-convert-qoi/-/blob/master/ff2qoi.c?ref_type=heads
   - Sixel
      - http://justsolve.archiveteam.org/wiki/Sixel
      - https://www.vt100.net/docs/vt3xx-gp/chapter14.html
   - XBM
      - http://justsolve.archiveteam.org/wiki/XBM
      - http://justsolve.archiveteam.org/wiki/XPM
   - ZX Spectrum
      - https://en.wikipedia.org/wiki/ZX_Spectrum_graphic_modes

Sources:
   - https://tools.suckless.org/farbfeld/
   - http://zzo38computer.org/fossil/farbfeld.ui/home
