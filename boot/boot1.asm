bits 16

global entry
extern boot2

entry:
    dd 0x906612EB

meminfo:
.lo: ; Memory below 1M
    dd 0x00000000
.hi: ; Memory above 1M
    dd 0x00000000
.hilo: ; Memory from 1M to 16M
    dd 0x00000000
.hihi: ; Memory above 16M
    dd 0x00000000

boot1_16:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    call print
    db `Tidying up...\r\n\0`

; Get the contiguous low memory size
query_lomem:
    clc
    int 0x12
    jc .fail
    and eax, 0xFFFF
    shl eax, 10
    mov [meminfo.lo], eax
    jmp .done
.fail:
    call print
    db `Argh!\r\nYour computer be right fucked.\r\n - Software Pirates\0`
    jmp $
.done:

; Get the contiguous high and higher memory sizes
query_himem:
    xor cx, cx
    xor dx, dx
    mov ax, 0xE801
    int 0x15
    jc .fail
    cmp ah, 0x86
    je .fail
    cmp ah, 0x80
    je .fail
    jcxz .safe
    mov ax, cx
    mov bx, dx
.safe:
    and eax, 0xFFFF
    shl eax, 10
    mov [meminfo.hilo], eax
    and ebx, 0xFFFF
    shl ebx, 16
    mov [meminfo.hihi], ebx
    add eax, ebx
    mov [meminfo.hi], eax
    jmp .done
.fail:
    call print
    db `Argh!\r\nYour computer be fucked in a very particuler manner.\r\n - Software Pirates\0`
    jmp $
.done:

enter_pmode:
.a20_1:
    in al, 0x64
    test al, 0x02
    jnz .a20_1
    mov al, 0xD1
    out 0x64, al
.a20_2:
    in al, 0x64
    test al, 0x02
    jnz .a20_2
    mov al, 0xDF
    out 0x60, al
    mov ax, 0x0003
    int 0x10
    lgdt [gdtr]
    mov eax, cr0
    or al, 0x01
    mov cr0, eax
    jmp 0x0008:boot1_32

print.loop:
    mov ah, 0x0E
    int 0x10
print:
    pop si
    lodsb
    push si
    cmp al, 0
    jne print.loop
    ret

align 4
gdt:
    dd 0x00000000
    dd 0x00000000
    dd 0x0000FFFF
    dd 0x00CF9A00
    dd 0x0000FFFF
    dd 0x00CF9200
gdtr:
    dw gdtr - gdt - 1
    dd gdt

bits 32

boot1_32:
    mov ax, 0x0010
    mov ds, ax
    mov es, ax
    mov ss, ax
    xor ax, ax
    mov fs, ax
    mov gs, ax
    mov esp, 0x7C00
    mov eax, 0xAAAAAAAF
    call boot2
    mov eax, 0xAAAAAAA0
    jmp $
