.CODE
PUBLIC RunAssemblyCode

RunAssemblyCode PROC
    mov rax, qword ptr [rsp+58h]
    mov [rsi+0000090], rax
    jmp qword ptr [rsp+4]
    ret
RunAssemblyCode ENDP
END