/* Find the first set bit
 * i.e. least signifigant 1 bit:
 * 0 => 0
 * 1 => 1
 * 2 => 2
 * 3 => 1
 * 4 => 3
 */

int
ffs(unsigned int mask)
{
	int i;

	if (!mask)
		return 0;
	i = 1;
	while (!(mask & 1)){
		i++;
		mask = mask >> 1;
	}
	return i;
}
