" Vim compiler file
" Compiler:         Devpac3 assembler
" Maintainer:       Simon Laszcz
" Latest Revision:  2024-05-03

if exists("current_compiler")
  finish
endif
let current_compiler = "devpac"

let s:cpo_save = &cpo
set cpo&vim

CompilerSet errorformat=
	\%EError:\ %m\ at\ line\ %l\ in\ file\ %f,
	\%WWarning:\ %m\ at\ line\ %l\ in\ file\ %f,
	\%Z%.%#\ \ \ \ \ %#%m,
	\%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
