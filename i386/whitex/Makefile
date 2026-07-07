# Kaynakları manuel tanımlayalım ki belirsizlik kalmasın
C_SRCS := $(wildcard *.c)
ASM_SRCS := $(wildcard *.asm)

# Objeleri out/ klasörüne yönlendiriyoruz
C_OBJS := $(patsubst %.c,out/%.o,$(C_SRCS))
ASM_OBJS := $(patsubst %.asm,out/%.o,$(ASM_SRCS))
OBJS := $(C_OBJS) $(ASM_OBJS)

TARGET := kernelll
OUT_DIR := out

all: $(OUT_DIR) $(TARGET)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

# .c -> .o
out/%.o: %.c
	$(CC) $(FINAL_CFLAGS) -c $< -o $@

# .asm -> .o
out/%.o: %.asm
	nasm -f elf32 $< -o $@

# Linker işlemi
$(TARGET): $(OBJS)
	$(LD) $(FINAL_LDFLAGS) -T linker.ld -o $@ $^

clean:
	rm -rf $(OUT_DIR) $(TARGET)

.PHONY: all clean
