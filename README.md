## Vim 7.4 for ATARI ST/TOS 

[View the original README](README.txt)

The following [help document](runtime_tos/doc/os_atari.txt) can also be viewed in Vim using 

`:help atari`

```
*os_atari.txt*	For Vim version 7.4.  Last change: Sep 2024

                         Notes on the Atari TOS Port

|TOS_01| Introduction
|TOS_02| Installation
|TOS_03| Using a Shell
|TOS_04| Setting the Palette
|TOS_05| Changing Resolution
|TOS_06| Changing the Keyboard Repeat Rate
|TOS_07| Development

==============================================================================

*TOS_01* Introduction					*atari*

The distribution contains tiny, small and normal feature builds. The
executables are named vimt.ttp, vims.ttp and vim.ttp respectively. There are
also 68020-60 builds of each.

vimt.ttp starts and quits much faster than vim.ttp and is also much smaller
(~450Kb vs. ~1Mb) but has far fewer features (e.g. splits, command windows and
eval are missing).

Use the :version command to check what features a build has.

==============================================================================

*TOS_02* Installation

The distribution includes a cut down runtime. You may use the full Vim runtime
(or add files to it) if you wish but having fewer files to glob improves 
performance.

You should set the following environment variables in your shell:
VIM
VIMRUNTIME
HOME
TMP

However, if not set the following defaults are used:
VIM=C:\VIM
VIMRUNTIME=C:\VIM\RT

The following variables are available in the normal build (which supports the
'eval' feature):

*v:resolution*	The current resolution. 
		Either st-{low,med,hi}, tt-{low,med,hi} or a Falcon mode 
		such as 320x200, 384x240 etc
*v:machine*	The current machine.
		Either ST, STe, TT, Falcon or unknown
*v:os*		The current OS.
		Either TOS, MagiC, MiNT or Geneva

They could be used to conditionally set options in your vimrc. 

You may need to test for the availability of a feature in vimrc. Do this 
using has(), e.g.

if has("eval")
  compiler DEVPAC
  color BLUE
endif

if has("eval") && v:os=="TOS"
  " set options, define functions. Whatever you want
endif

ATARI Vim looks for vimrc files in the usual places. Check where these are
using the :version command.

It is recommended to store user files in $HOME\vimfiles.

==============================================================================

*TOS_03* Using a Shell					*shell* *gulam*

Any shell that sets the _shell_p vector can be used, e.g. Gulam.

:lmake and :lopen can be used in vim.ttp. By default Vim attempts to execute
the make utility in the current working directory. If make is not installed,
the simplest way to get this to work is to create a shell script.

e.g. 'make.g' might contain:

d:\devpac\gen.ttp main.s

You will also need to set the make program like so:

	:set makeprg=make.g

Then to make, use:

	:lmake

The cut down runtime includes compiler files for gcc, devpac and vasm. These
files will format the error output from these programs for display by :lopen.

For devpac you will need to set the default compiler first. Do this with the 
command:

	:compiler DEVPAC
	:compiler<SP><TAB>	to enumerate them

==============================================================================

*TOS_04* Setting the Palette		*paltos* *palvim* *palmap* *sane*

Vim colours are numbered thus:

0	Black
1	DarkBlue
2	DarkGreen
3	DarkCyan
4	DarkRed
5	DarkMagenta
6	Brown
7	LightGray
8	DarkGray
9	Blue
10	Green
11	Cyan
12	Red
13	Magenta
14	Yellow
15	White

When ATARI Vim starts it assumes that a default ST palette is set and maps Vim
colours to ST colours as best it can.

The distribution includes many Vim colour schemes. Some target 256 colour
xterm displays so will not display perfectly. Tab through all colour schemes
with:

	:color<SP><TAB>

For example, to set the blue scheme use:

	:color blue

Remove a colour scheme using:

	:color DEFAULT

You can check the current highlighting scheme using the :hi command. You may
wish to override some highlighting options, e.g.

	:hi Search ctermbg=0 ctermfg=15

:palvim (single TOS only)

Better colour compatibility is achieved by changing the ST palette to match
Vim. A colour remapping occurs to ensure that the Vim background colour
matches the border colour.  When using a resolution with fewer than 16 
colours an additional colour remapping takes place. This port supports a 
maximum of 16 colours even on the TT and Falcon.

When Vim is quit, the startup palette is restored.

:paltos (single TOS only)

This sets a default ST palette, potentially overriding the palette set when
Vim started.

When Vim is quit, the startup palette is restored.

:palmap {16 hex char string}

You may map your existing palette to the Vim colour numbers by supplying a 16
character hex string; one hex digit per colour indexes 0 through to 15.

e.g.

	:palmap abc0123456789def

will map colour index 10 to Vim colour number 0 etc

:sane

Will restore your palette and resolution to their startup values.

==============================================================================

*TOS_05* Changing Resolution					*rez*

:rez		will display the current resolution.
:rez name	will set the named resolution.
:rez<SP><TAB>	will allow you to <TAB> through all valid resolutions.
:sane		will restore your palette and resolution to their startup
		values.

When Vim is quit, the startup resolution is restored.

The current resolution is stored in the variable |v:resolution|.

==============================================================================

*TOS_06* Changing the Keyboard Repeat Rate			*Kbrate*

:Kbrate			Display the current setting.
:Kbrate initial every	Set keypresses to repeat after an initial
			number of ticks and then every n 50/60Hz ticks.

Typically, having multiple split windows open will slow down Vim. When Vim is 
slow you may find it useful to alter the Kbrate, particularly the 'every'
value. Increasing it prevents the input buffer becoming too full and makes
Vim feel more responsive.

When Vim is quit, the startup keyboard rate is restored.

==============================================================================

*TOS_07* Development						*develop*

Changes to the existing code are wrapped with the 'TOS' macro.

All os_atari_* files and Make_tos.mak are specific to this port.

==============================================================================

vim:tw=78:ts=8:ft=help:norl:noexpandtab:
```
