// nidhuggc: -unroll=2 -sc
int val_count, val2_count;

struct array {
    unsigned long *value[2];
};

void problematic(struct array *ap) {
    for (int i = val_count - 1; i >= 0; i--) {
	unsigned long *vp = ap->value[i];
	for (int j = 0; j < val2_count; j++, vp++) {
	    *vp = 42;
	}
    }
}

int main() {
    problematic(((void*)0));

    return 0;
}
