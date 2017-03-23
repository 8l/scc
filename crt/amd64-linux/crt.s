	.file	"crt.as"
	.text
	.global	_start

_start:
	call	main
	movl    %eax, %edi
	call	exit
