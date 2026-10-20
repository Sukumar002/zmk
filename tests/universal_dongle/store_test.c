#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <zmk/universal_store.h>
int main(void) {
 struct ud_store s; ud_store_init(&s);
 const char hana[]="HANA_A_SHIFT", chocofi[]="CHOCOFI_A_SHIFT";
 assert(strcmp(ud_store_key(UD_STORE_HANA,UD_STORE_KEYMAP),ud_store_key(UD_STORE_CHOCOFI,UD_STORE_KEYMAP))!=0);
 assert(ud_store_write(&s,UD_STORE_HANA,UD_STORE_KEYMAP,0,hana,sizeof(hana))==UD_STORE_OK);
 assert(ud_store_write(&s,UD_STORE_CHOCOFI,UD_STORE_KEYMAP,0,chocofi,sizeof(chocofi))==UD_STORE_OK);
 assert(!strcmp((const char*)ud_store_read(&s,UD_STORE_HANA,UD_STORE_KEYMAP)->data,hana));
 assert(!strcmp((const char*)ud_store_read(&s,UD_STORE_CHOCOFI,UD_STORE_KEYMAP)->data,chocofi));
 assert(ud_store_write(&s,UD_STORE_HANA,UD_STORE_KEYMAP,0,"bad",4)==UD_STORE_CONFLICT);
 assert(ud_store_write(&s,UD_STORE_CHOCOFI,UD_STORE_COMBOS,0,"jk",3)==UD_STORE_OK);
 assert(ud_store_read(&s,UD_STORE_HANA,UD_STORE_COMBOS)->length==0);
 assert(ud_store_write(&s,UD_STORE_CHOCOFI,UD_STORE_KEYMAP,1,"new",4)==UD_STORE_OK);
 assert(!strcmp((const char*)ud_store_read(&s,UD_STORE_HANA,UD_STORE_KEYMAP)->data,hana));
 puts("board-scoped storage isolation tests passed");
 return 0;
}
