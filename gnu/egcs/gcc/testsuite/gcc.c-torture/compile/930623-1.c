g (a, b) {}

f (xx)
     void* xx;
{
  __builtin_apply ((void*)g, xx, 200);
}
