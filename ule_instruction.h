enum ule_instruction_type {
	ULE_ADD,
	ULE_MUL,
	ULE_COPY
}

typedef struct {
	ule_instruction_type type;
} ule_instruction;
