foo (x, c)
{
  return x << -c;
}

main ()
{
  printf ("%x\n", foo (0xf05, -4));
}
