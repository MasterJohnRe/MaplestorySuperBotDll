.CODE
PUBLIC RunAssemblyCode

RunAssemblyCode PROC
    mov rax, qword ptr [rsp+58h]
    jmp qword ptr [rsp+4]
    ret
RunAssemblyCode ENDP
END