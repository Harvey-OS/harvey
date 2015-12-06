#!/bin/sh
#sed -i 's/nil/NULL/g' $1

# stuff I'm not smart enough to do with spatch

# spatch can't do this yet -- we've talked to them.
sed -i 's/struct ipifc\*,/struct ipifc *unused_ipifc,/' $1
sed -i 's/(struct ipifc\*)/(struct ipifc *unused_ipifc)/' $1
sed -i 's/ *uint8_t,/ uint8_t unused_uint8_t,/' $1
sed -i 's/ *uint8_t\*,/ uint8_t *unused_uint8_p_t,/' $1
sed -i 's/ *uint8_t)/ uint8_t unused_uint8_t)/' $1
sed -i 's/ *uint8_t\*,/ uint8_t *unused_uint8_p_t,/' $1
sed -i 's/ *void\*,/ void *unused_voidp,/' $1
sed -i 's/ *uint8_t)/ uint8_t unused_uint8_t)/' $1
sed -i 's/ *uint8_t\*)/ uint8_t *unused_uint8_p_t)/' $1
sed -i 's/ *void\*)/ void *unused_voidp)/' $1
sed -i 's/ *char\*\([,)]\)/ char *unused_char_p_t\1/' $1
sed -i 's/ *char\*\*\([,)]\)/ char **unused_char_pp_t\1/' $1
sed -i 's/ *int,/ int unused_int,/' $1
sed -i '/USED(.*);/d' $1

