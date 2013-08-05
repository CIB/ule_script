enum ule_object_type {
	ULE_NIL,
	ULE_INTEGER,
	ULE_FLOAT,
	ULE_BOOL,
	ULE_STRING,
	ULE_ARRAY,
	ULE_TABLE
};

struct ule_array {
	
}

struct ule_table {
	
}

struct ule_object {
	ule_object_type type;
	
	union {
		const char* string_;
		float float_;
		int int_;
		ule_array array_;
		ule_table table_;
	}
} ule_object;
