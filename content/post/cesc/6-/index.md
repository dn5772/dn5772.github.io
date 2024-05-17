---
title: '5-rev'
description: assembly
date: 2024-05-17
tags: 
    - assembly
    - summry
categories:
    - neovim
---

## 6-리버싱 기초

### 6.1 스택 구조 분석

``` call 0x80483b3 <add> ```
push eip + jump 0x80483b3와 같음\
add 함수를 실행 시킬때

1. push %ebp
2. mov %esp, %ebp
3. sub $0xc, %esp
4. movl $0x2, 0x4(%esp)
5. movl $0x1,(%esp)
6. call 0x80483b3
   1. d
   2. 
7. push %ebp

함수가 호출 될 때
```
push %ebp
mov %esp, %ebp

leave (mov %esp, %ebp	pop %ebp)
ret
```
은 한쌍임 

- 함수 호출 직후 시점의 esp에 함수의 리턴 장소 저장
- 함수 호출 후에도 ebp를 통해 리턴 함수 스택 영역에 접근 가능
- ebp+4의 위치(바로 아래)에 항상 현재 함수 리턴 장소가 있음

함수는 항상 호출한 쪽이 파라미터 쌓고 호출, 파라미터 정리함

### 시스템 콜

- 함수 파라미터를 사용자 stack에 넣음
- libc.so
  - 함수 파라미터를 ebx, ecx, edx에 차례로 복사
  - 시스템콜 번호를 eax에 저장
  - 인터렙트 또는 sysenter 발생
- system_call()
  - 커널 모드 진입


#### virtuual system call

INT 80, sysenter를 호출 하는데 cpu지원 여부에 따라 커널이 선택하도록 함

- 커널이 0xFFFFE000에 VDSO를 맵핑
- 커널이 sysenter를 지원하는지 확인
- 지원하면 sysenter.S를 복사, 아니면 int80.S복사