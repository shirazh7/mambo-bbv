MAMBO: A Low-Overhead Dynamic Binary Modification Tool for ARM
==============================================================

HOW TO RUN TEST CASE:
1. Connect to Virtual Machine
2. Run gdb --args ./dbm helloworld (expecting two system calls)
3. Create breakpoint "b bbv_exe"
4. Change layout: "layout regs"
5. Run the program using "run"
6. Use "step" to iterate through the progamme
7. After 9 steps, you should see that the "write" system call is stored in register X7 when the program is in the "inc_bbv_syscall_in_list" function

