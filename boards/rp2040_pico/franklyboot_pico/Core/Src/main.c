int main(void)
{
    for(;;) {
        asm volatile("nop");
    }
    return 0;
}