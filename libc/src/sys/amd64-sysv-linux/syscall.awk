# This job is very easy because app and kernel ABI are identical
# until the 4th parameter, so we only have to set the syscall
# number in rax

/^#/	{next}
	{name=$2 ".s"
	 printf ".global %s\n" \
	        "%s:\n" \
	        "\tmovq\t$%d,%%rax\n" \
	        "\tsyscall\n" \
	        "\tret\n", $2, $2, $1 > name
	 close(name)}
