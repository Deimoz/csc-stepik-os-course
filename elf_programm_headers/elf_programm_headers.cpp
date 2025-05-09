#define ELF_NIDENT	16

// program header-ы такого типа нужно загрузить в
// память при загрузке приложения
#define PT_LOAD		1

// структура заголовка ELF файла
struct elf_hdr {
	std::uint8_t e_ident[ELF_NIDENT];
	std::uint16_t e_type;
	std::uint16_t e_machine;
	std::uint32_t e_version;
	std::uint64_t e_entry;
	std::uint64_t e_phoff;
	std::uint64_t e_shoff;
	std::uint32_t e_flags;
	std::uint16_t e_ehsize;
	std::uint16_t e_phentsize;
	std::uint16_t e_phnum;
	std::uint16_t e_shentsize;
	std::uint16_t e_shnum;
	std::uint16_t e_shstrndx;
} __attribute__((packed));

// структура записи в таблице program header-ов
struct elf_phdr {
	std::uint32_t p_type;
	std::uint32_t p_flags;
	std::uint64_t p_offset;
	std::uint64_t p_vaddr;
	std::uint64_t p_paddr;
	std::uint64_t p_filesz;
	std::uint64_t p_memsz;
	std::uint64_t p_align;
} __attribute__((packed));



std::size_t space(const char *name)
{
    // Ваш код здесь, name - имя ELF файла, с которым вы работаете
    // вернуть нужно количество байт, необходимых, чтобы загрузить
    // приложение в память
    FILE *file = fopen(name, "rb");
    if (file == NULL) {
        return 0;
    }

    struct elf_hdr head;
    if (fread(&head, sizeof(struct elf_hdr), 1, file) != 1) {
        fclose(file);
        exit(1);
    }
    
    fseek(file, head.e_phoff, SEEK_SET);
    
    struct elf_phdr pheads[head.e_phnum];
    size_t read_count = fread(pheads, sizeof(struct elf_phdr), head.e_phnum, file);
    fclose(file);
    
    if (read_count != head.e_phnum) {
        exit(2);
    }
        
    std::size_t sum_size = 0;
    for (int i = 0; i < head.e_phnum; i++) {
        if (pheads[i].p_type != PT_LOAD) {
            continue;
        }
        sum_size += pheads[i].p_memsz;
    }
    
    return sum_size;
}