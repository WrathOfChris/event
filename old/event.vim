" Vim syntax file
" Language:		EVENT
" Maintainer:	Christopher Maxwell <maxwell@neologi.ca>
" URL:		
" Last Change:	2006 Apr 14

" Use the following to enable in .vimrc
"
"	augroup filetypedetect
"		au BufNewFile,BufRead *.event set filetype=event formatoptions=croql cindent \
"			comments=sr:/*,mb:*,el:*/,://
" 	augroup END
"	au FileType event setf event

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

runtime syntax/c.vim

" keyword definitions
syn keyword evTodo			contained TODO FIXME XXX
syn keyword evExternal		DEFINITIONS BEGIN END FROM FUNCTION INCLUDE
syn keyword evExternal		PREAMBLE INTERFACE ACTION FINISH
syn keyword evFieldOption	CONSTANT REFERENCE CONSTRUCTED OPTIONAL
syn keyword evFunction		CALL CODE CONTINUE COPY RETRY RETURN SYNC TEST WAIT
syn keyword evFunction		CALLBACK FORK FORKWAIT
syn keyword evVariable		IN OUT RET STACK
syn keyword evVariable		CBARG CBFUNC ERRORCODE
syn keyword evNumber		NULL

" Strings and constants
syn match	evSpecial		contained "\\\d\d\d\|\\."
syn region	evString		start=+"+  skip=+\\\\\|\\"+  end=+"+  contains=evSpecial
syn match	evCharacter		"'[^\\]'"
syn match	evSpecialCharacter "'\\.'"
syn match	evNumber		"-\=\<\d\+L\=\>\|0[xX][0-9a-fA-F]\+\>"
syn match	evLineComment	"--.*" contains=evTodo
syn match	evLineComment	"--.*--" contains=evTodo
syn match	evLineComment	"^#.*" contains=evTodo
syn match	evLineComment	"\s#.*"ms=s+1 contains=evTodo

syn match	evDefinition 	"^\s*[a-zA-Z][-a-zA-Z0-9_.\[\] \t{}]* *::="me=e-3 contains=evType
syn match	evBraces     	"[{}]"

syn sync ccomment evComment

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_ev_syn_inits")
  if version < 508
    let did_ev_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif
  HiLink evDefinition	Function
  HiLink evBraces		Function
  HiLink evStructure	Statement
  HiLink evBoolValue	Boolean
  HiLink evSpecial		Special
  HiLink evString		String
  HiLink evCharacter	Character
  HiLink evSpecialCharacter	evSpecial
  HiLink evNumber		evValue
  HiLink evComment		Comment
  HiLink evLineComment	evComment
  HiLink evType			Type
  HiLink evTypeInfo		PreProc
  HiLink evValue		Number
  HiLink evExternal		Include
  HiLink evTagModifier	Function
  HiLink evFieldOption	Type

  HiLink evFunction		Statement
  HiLink evTodo			Todo
  HiLink evVariable		Identifier
  delcommand HiLink
endif

let b:current_syntax = "ev"
