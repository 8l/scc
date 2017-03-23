int
main()
{
	int i, *p = &i;

	return p - (void*) 0 == 0;
}
