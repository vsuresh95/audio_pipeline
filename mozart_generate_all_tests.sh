# Software
ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-software.exe

# Monolithic MESI
DO_CHAIN_OFFLOAD=1 USE_MONOLITHIC_ACC=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-mono-os.exe
DO_NP_CHAIN_OFFLOAD=1 USE_MONOLITHIC_ACC=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-mono-chaining-mesi.exe
DO_PP_CHAIN_OFFLOAD=1 USE_MONOLITHIC_ACC=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-mono-pipelining-mesi.exe

# Monolithic DMA
COH_MODE=1 DO_NP_CHAIN_OFFLOAD=1 USE_MONOLITHIC_ACC=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-mono-chaining-dma.exe
COH_MODE=1 DO_PP_CHAIN_OFFLOAD=1 USE_MONOLITHIC_ACC=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-mono-pipelining-dma.exe

# Monolithic SPX
IS_ESP=0 COH_MODE=2 DO_NP_CHAIN_OFFLOAD=1 USE_MONOLITHIC_ACC=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-mono-chaining-spandex.exe
IS_ESP=0 COH_MODE=2 DO_PP_CHAIN_OFFLOAD=1 USE_MONOLITHIC_ACC=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-mono-pipelining-spandex.exe

# Disaggregated MESI
DO_CHAIN_OFFLOAD=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-disag-os.exe
DO_NP_CHAIN_OFFLOAD=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-disag-chaining-mesi.exe
DO_PP_CHAIN_OFFLOAD=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-disag-pipelining-mesi.exe

# Disaggregated DMA
COH_MODE=1 DO_NP_CHAIN_OFFLOAD=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-disag-chaining-dma.exe
COH_MODE=1 DO_PP_CHAIN_OFFLOAD=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-disag-pipelining-dma.exe

# Disaggregated SPX
IS_ESP=0 COH_MODE=2 DO_NP_CHAIN_OFFLOAD=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-disag-chaining-spandex.exe
IS_ESP=0 COH_MODE=2 DO_PP_CHAIN_OFFLOAD=1 ESP_ROOT=$ESP_ROOT CC=riscv64-unknown-linux-gnu-gcc CXX=riscv64-unknown-linux-gnu-g++ LD=riscv64-unknown-linux-gnu-g++ make solo.opt.exe
mv solo.opt.exe audio-disag-pipelining-spandex.exe