static int test_value = { 2 };

int main(void)
{
    for(;;) {
        asm volatile("nop");
        test_value++;
    }
    return 0;
}