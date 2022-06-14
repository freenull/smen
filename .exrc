" project specific vim/neovim config
let g:ale_cpp_cc_executable = "clang"
let g:ale_c_cc_options = "-std=c++20 -Weverything -Wno-inconsistent-missing-destructor-override -Wno-covered-switch-default -Wno-weak-vtables -Wno-padded -Wno-missing-prototypes -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-exit-time-destructors -Wno-global-constructors -Wno-shadow-field-in-constructor -Wno-shadow-field -I include -I subprojects/imgui-1.87"
let g:ale_cpp_cc_options = g:ale_c_cc_options
