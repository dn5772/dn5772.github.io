---
title: '[Mac] Neonim 설치'
description: Neovim
date: 2024-04-22
tags: 
    - neovim
    - vim
    - install
    - setting
categories:
    - neovim
---

## [Mac]Neonim-설치

### 네오빔
자기전에 유튜브 보다가 neovim이라는 IDE를 알게됐다.\
내가 주로사용하는 IDE는 VScode와 Pychram인데 neovim이 너무 맘에 들어서 갈아타려고 마음 먹었다.

1학년 입학하자마자 C언어 보다 먼저 배운게 vi 사용법 이었기 때문에 vi 사용법도 어느정도 알고있고, 무엇보다 간지가 나서 써보고 싶었다.\
간지가 가장 큰 이유이지만, 다른 이유들도 적어보자면

1. 간지가 난다
2. 다양한 플러그인
3. 키보드만 써서 코딩
4. 가볍고 빠름
5. 커스터마이징 자유도 높음

### Neovim 설치

```zsh
brew install neovim
```
설치는 간단하다
```zsh
nvim
```
`nvim` 명령어로 실행시켜볼 수 있다

### 기본 설정 파일 생성

Neovim 설정은 `~/.config/nvim/init.vim` 또는 `init.lua`에 저장합니다. (Neovim 0.5+부터 Lua 지원)

먼저 디렉토리 생성:
```zsh
mkdir -p ~/.config/nvim
```

기본 `init.vim` 파일 생성:
```zsh
touch ~/.config/nvim/init.vim
```

### vim-plug 설치

플러그인 매니저로 vim-plug를 사용합니다.

```zsh
sh -c 'curl -fLo "${XDG_DATA_HOME:-$HOME/.local/share}"/nvim/site/autoload/plug.vim --create-dirs \
      https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim'
```

### 추천 플러그인 설정

`init.vim`에 플러그인들을 추가합니다:

```vim
call plug#begin('~/.vim/plugged')

" 파일 탐색기
Plug 'preservim/nerdtree'
Plug 'Xuyuanp/nerdtree-git-plugin'

" 테마
Plug 'morhetz/gruvbox'
Plug 'dracula/vim', { 'as': 'dracula' }

" 상태바
Plug 'vim-airline/vim-airline'
Plug 'vim-airline/vim-airline-themes'

" 코드 하이라이트
Plug 'sheerun/vim-polyglot'

" 자동완성
Plug 'neoclide/coc.nvim', {'branch': 'release'}

" Git 통합
Plug 'tpope/vim-fugitive'

" 주석 토글
Plug 'tpope/vim-commentary'

" 괄호 자동완성
Plug 'jiangmiao/auto-pairs'

" 파일 검색
Plug 'junegunn/fzf', { 'do': { -> fzf#install() } }
Plug 'junegunn/fzf.vim'

call plug#end()
```

플러그인 설치: Neovim에서 `:PlugInstall` 실행

### 기본 설정 추가

`init.vim`에 기본 설정들 추가:

```vim
" 기본 설정
set number              " 라인 넘버 표시
set relativenumber      " 상대적 라인 넘버
set tabstop=4           " 탭 크기
set shiftwidth=4        " 들여쓰기 크기
set expandtab           " 탭을 스페이스로 변환
set smartindent         " 스마트 들여쓰기
set mouse=a             " 마우스 지원
set clipboard=unnamedplus " 시스템 클립보드 사용

" 검색 설정
set ignorecase          " 대소문자 무시
set smartcase           " 대문자 포함 시 대소문자 구분
set incsearch           " 점진적 검색
set hlsearch            " 검색 결과 하이라이트

" 테마 설정
colorscheme gruvbox
set background=dark

" 키 매핑
let mapleader = " "     " 리더 키를 스페이스로 설정
nmap <leader>nt :NERDTreeToggle<CR>
nmap <leader>ff :Files<CR>
nmap <leader>fg :GFiles<CR>
nmap <leader>fb :Buffers<CR>
```

### NERDTree
`:NERDTreeToggle`로 키고 끌수있음

- 아이콘 깨질 때
```
brew tap homebrew/cask-fonts
brew install font-hack-nerd-font
```
로 폰트 설치후 터미널 폰트 ```Hack Nerd Font Mono```로 변경

### CoC (Conquer of Completion) 설정

자동완성 플러그인 CoC를 위한 추가 설정:

```vim
" CoC 설정
let g:coc_global_extensions = [
  \ 'coc-tsserver',
  \ 'coc-python',
  \ 'coc-json',
  \ 'coc-html',
  \ 'coc-css'
  \ ]

" CoC 키 매핑
inoremap <silent><expr> <TAB>
      \ pumvisible() ? "\<C-n>" :
      \ <SID>check_back_space() ? "\<TAB>" :
      \ coc#refresh()
inoremap <expr><S-TAB> pumvisible() ? "\<C-p>" : "\<C-h>"

function! s:check_back_space() abort
  let col = col('.') - 1
  return !col || getline('.')[col - 1]  =~# '\s'
endfunction

" GoTo code navigation
nmap <silent> gd <Plug>(coc-definition)
nmap <silent> gy <Plug>(coc-type-definition)
nmap <silent> gi <Plug>(coc-implementation)
nmap <silent> gr <Plug>(coc-references)
```

### Lua 설정으로 마이그레이션 (선택사항)

Neovim 0.5+부터 Lua 설정을 사용할 수 있습니다. `init.lua`로 변환:

```lua
-- init.lua
require('plugins')  -- 플러그인 설정 분리
require('settings') -- 기본 설정
require('keymaps')  -- 키 매핑
```

### 추가 팁

1. **터미널 통합**: Neovim 내에서 터미널 사용 - `:term`
2. **세션 관리**: 작업 세션 저장 - `:mksession`
3. **매크로**: 반복 작업 자동화 - `q`로 녹화 시작
4. **레지스터**: 복사/붙여넣기 고급 사용 - `:reg`로 확인
5. **vimrc 리로드**: 설정 변경 후 `:source ~/.config/nvim/init.vim`

### 자주 사용하는 명령어

- `:w` - 저장
- `:q` - 종료
- `:wq` - 저장 후 종료
- `:e 파일명` - 파일 열기
- `/검색어` - 검색
- `:%s/old/new/g` - 전체 치환

Neovim은 처음에 어렵지만, 익숙해지면 생산성이 엄청나게 올라갑니다. 꾸준히 사용해보세요!