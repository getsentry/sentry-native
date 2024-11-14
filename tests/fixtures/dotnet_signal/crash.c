void native_crash(void)
{
    *(int *)10 = 100;
}