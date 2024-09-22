" Vim compiler file
" Compiler:         vasm assembler
" Maintainer:       Simon Laszcz
" Latest Revision:  2024-09-27

if exists("current_compiler")
  finish
endif
let current_compiler = "vasm"

let s:cpo_save = &cpo
set cpo&vim

CompilerSet errorformat=%E%.%#error\ %n\ in\ line\ %l\ of\ \"%f\":\ %m,%Z\>%m,%-G%.%#

let &cpo = s:cpo_save
unlet s:cpo_save
