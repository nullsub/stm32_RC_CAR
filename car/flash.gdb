target remote :1234
file build/template.elf
load build/template.elf
delete breakpoints
c

