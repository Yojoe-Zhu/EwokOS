VFSD_DIR = sbin/vfsd
VFSD_PROGRAM = build/sbin/vfsd
PROGRAM += $(VFSD_PROGRAM)

EXTRA_CLEAN += $(VFSD_PROGRAM) $(VFSD_DIR)/*.o

$(VFSD_PROGRAM): $(VFSD_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	$(CC) $(CFLAGS) -I $(VFSD_DIR) -c -o $(VFSD_DIR)/tree.o $(VFSD_DIR)/tree.c
	$(CC) $(CFLAGS) -I $(VFSD_DIR) -c -o $(VFSD_DIR)/fstree.o $(VFSD_DIR)/fstree.c
	$(CC) $(CFLAGS) -I $(VFSD_DIR) -c -o $(VFSD_DIR)/vfsd.o $(VFSD_DIR)/vfsd.c
	$(CC) $(CFLAGS) -I $(VFSD_DIR) -c -o $(VFSD_DIR)/pipe.o $(VFSD_DIR)/pipe.c
	$(LD) -Ttext=100 $(VFSD_DIR)/vfsd.o \
		$(VFSD_DIR)/tree.o \
		$(VFSD_DIR)/pipe.o \
		$(VFSD_DIR)/fstree.o \
		lib/libewoklibc.a \
		$(COMMON_OBJ) \
		-o $(VFSD_PROGRAM)
