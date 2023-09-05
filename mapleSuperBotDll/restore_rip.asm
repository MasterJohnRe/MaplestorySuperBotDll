.CODE
PUBLIC RunAssemblyCode

RunAssemblyCode PROC
    mov rax, qword ptr [rsp+58h]
    mov [rsi+0000090], rax
    jmp rcx
    ret
RunAssemblyCode ENDP
END