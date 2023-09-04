org 0x7C00

bits 16

dd 0x906616EB

ekfs:
.magic:
    dd 0x00000000
.disk_blocks:
    dd 0x00000000
.root_blocks:
    dd 0x00000000
.skip_blocks:
    dd 0x00000000
.disk_id:
    dd 0x00000000

jmp 0x0000:entry

entry:
    cli
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

; Load the root directory to 0x7E00-0x7FFFF (~480kb)
load_root:
    mov ecx, [ekfs.root_blocks]
    mov eax, [ekfs.disk_blocks]
    shr eax, 7
    add eax, [ekfs.skip_blocks]
    xor bx, bx
    mov di, 0x7E00
    call read

; Search the root directory for `boot`
search_root:
    xor eax, eax
    mov bx, 0x7E00
    xor ebp, ebp
    mov ecx, [ekfs.root_blocks]
    shl ecx, 3
.search_entry:
    cmp dword [bx], 0x00000000
    je .not_found
    cmp eax, ecx
    jnl .not_found
    cmp dword [bx], 0xFFFFFFFF
    jne .next_entry
    cmp byte [bx + 31], 0x00
    jne .next_entry
    lea di, [bx + 32]
    call is_boot
    jz .found
.next_entry:
    inc eax
    add bx, 64
    jmp .search_entry
.not_found:
    call print
    db `Erm... What the scallop!\0`
    jmp $
.found:
    mov edx, [bx + 4] ; Save the target ID for loading later

; Load the allocation table to 0x7E00-0x7FFFF (~480kb)
load_atbl:
    mov ecx, [ekfs.disk_blocks]
    shr ecx, 7
    mov eax, [ekfs.skip_blocks]
    xor bx, bx
    mov di, 0x7E00
    call read

; Load the bootloader to 0x0600-0x7BFF (~30kb)
load_boot:
    mov eax, edx ; Target ID that was saved from earlier
    mov di, 0x0600
    mov word [read.len], 0x0001
    mov word [read.base], 0x0000
.next_block:
    mov [read.disp], di
    mov [read.lba], eax
    call read_block
    add di, 0x0200
    mov ebx, eax
    shl ebx, 2
    add ebx, 0x7E00
    mov eax, [ebx]
    cmp eax, 0xFFFFFFFF
    jne .next_block

; Enter the bootloader
    jmp 0x0600

; Prints out the string starting after the caller
; Code executes like normal after
print.loop:
    mov ah, 0x0E
    int 0x10
print:
    pop si
    lodsb
    push si
    cmp al, 0
    jne .loop
    ret

; is_boot(x) -> strncmp(x, `boot\0`, 5)
is_boot:
    push ecx
    push esi
    mov cx, 5
    mov si, .file_name
    repe cmpsb
    pop esi
    pop ecx
    ret
.file_name:
    db `boot\0`

; Reads multiple blocks into memory
; ecx: How many blocks to read
; eax: Where (on disk) to read from
; bx:  Where (in memory) to read to (segment)
; di:  Where (in memory) to read to (offset)
read:
    mov word [read.len], 0x0001
.next_block:
    mov [read.base], bx
    mov [read.disp], di
    mov [read.lba], eax
    call read_block
    add di, 0x0200
    jnc .no_carry
    add bx, 0x1000
.no_carry:
    inc eax
    loop .next_block
    ret

; Just a wrapper for int 0x13
read_block:
    push esi
    push eax
    push edx
    mov si, disk_packet
    mov ah, 0x42
    mov dl, 0x80
    int 0x13
    jc .fail
    pop edx
    pop eax
    pop esi
    ret
.fail:
    call print
    db `Are you sillius wight meow?\0`
    jmp $

align 2
disk_packet:
    dw 0x0010
read.len:
    dw 0x0000
read.disp:
    dw 0x0000
read.base:
    dw 0x0000
read.lba:
    dq 0x0000000000000000

times 510 + $$ - $ db 0
dw 0xAA55
