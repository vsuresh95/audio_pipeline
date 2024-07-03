# Monolithic ESP
DO_CHAIN_OFFLOAD=1 USE_MONOLITHIC_ACC=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-monolithic-os-mesi.exe
DO_NP_CHAIN_OFFLOAD=1 USE_MONOLITHIC_ACC=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-monolithic-asi-mesi.exe
DO_PP_CHAIN_OFFLOAD=1 USE_MONOLITHIC_ACC=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-monolithic-pipeline-mesi.exe

# Monolithic SPX
IS_ESP=0 COH_MODE=2 DO_CHAIN_OFFLOAD=1 USE_MONOLITHIC_ACC=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-monolithic-os-spandex.exe
IS_ESP=0 COH_MODE=2 DO_NP_CHAIN_OFFLOAD=1 USE_MONOLITHIC_ACC=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-monolithic-asi-spandex.exe
IS_ESP=0 COH_MODE=2 DO_PP_CHAIN_OFFLOAD=1 USE_MONOLITHIC_ACC=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-monolithic-pipeline-spandex.exe

# Disaggregated ESP
DO_CHAIN_OFFLOAD=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-disag-os-mesi.exe
DO_NP_CHAIN_OFFLOAD=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-disag-asi-mesi.exe
DO_PP_CHAIN_OFFLOAD=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-disag-pipeline-mesi.exe

# Disaggregated SPX
IS_ESP=0 COH_MODE=2 DO_CHAIN_OFFLOAD=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-disag-os-spandex.exe
IS_ESP=0 COH_MODE=2 DO_NP_CHAIN_OFFLOAD=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-disag-asi-spandex.exe
IS_ESP=0 COH_MODE=2 DO_PP_CHAIN_OFFLOAD=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-disag-pipeline-spandex.exe