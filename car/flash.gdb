target remote :4242
file build/template.elf
load build/template.elf
delete breakpoints
c
