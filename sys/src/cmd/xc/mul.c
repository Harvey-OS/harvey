#include "gc.h"

/*
 * find the encoded multiply instructions
 * for a constant v
 */
Multab*
mulcon0(long v)
{
	int a1, a2, g;

	if(v < 0)
		v = -v;
	a1 = 0;
	a2 = multabsize;
	for(;;) {
		if(a1 >= a2)
			return 0;
		g = (a2 + a1)/2;
		if(v < multab[g].val) {
			a2 = g;
			continue;
		}
		if(v > multab[g].val) {
			a1 = g+1;
			continue;
		}
		break;
	}
	return multab + g;
}

/*
 * code sequences for multiply by constant.
 * [a-n][0-1]
 *	sll	$(A-'a'),rbit1
 * [+][0-7]
 *	add	rbit1,rbit2,rbit4
 * [-][0-7]
 *	sub	rbit1,rbit2,rbit4
 */

Multab multab[] =
{
       3, "+4+1",
       5, "+4+1+1",
       6, "+4+1+0",
       7, "+4c1-2",
       9, "+4c1+1",
      10, "+4d0+1",
      11, "+4+1c1+1",
      12, "+4+1c0",
      13, "+4+1d1-2",
      14, "+4e0-1",
      15, "+4d1-2",
      17, "+4d1+1",
      18, "+4e0+1",
      19, "+4+1d1+1",
      20, "+4+1+1c0",
      21, "+4+1+1d1+1",
      22, "+4+1d0-1",
      23, "+4+5d1-2",
      24, "+4+1d0",
      25, "+4+5d1+1",
      26, "+4+1d0+1",
      27, "+4+1+1e1-2",
      28, "+4+3e1-2",
      29, "+4+1e1-2",
      30, "+4f0-1",
      31, "+4e1-2",
      33, "+4e1+1",
      34, "+4f0+1",
      35, "+4+1e1+1",
      36, "+4+3e1+1",
      37, "+4+1+1e1+1",
      38, "+4+1+0e1+1",
      39, "+4+7+5d1-2",
      40, "+4+1+1d0",
      41, "+4+7+5d1+1",
      42, "+4+1+1d0+1",
      43, "+4+1+5d1+1",
      44, "+4+1+7e0-1",
      45, "+4+1+4d1-2",
      46, "+4+1e0-1",
      47, "+4+5e1-2",
      48, "+4+1e0",
      49, "+4+5e1+1",
      50, "+4+1e0+1",
      51, "+4+1+4d1+1",
      52, "+4+1+7e0+1",
      53, "+4+1+5e0+1",
      54, "+4+1+4e0+1",
      55, "+4c1+1d1-2",
      56, "+4c1-2d0",
      57, "+4c1-1d1+1",
      58, "+4+1+0f1-2",
      59, "+4+1+1f1-2",
      60, "+4+3f1-2",
      61, "+4+1f1-2",
      62, "+4g0-1",
      63, "+4f1-2",
      65, "+4f1+1",
      66, "+4g0+1",
      67, "+4+1f1+1",
      68, "+4+3f1+1",
      69, "+4+1+1f1+1",
      70, "+4+1+0f1+1",
      71, "+4c1+5d1-2",
      72, "+4c1+1d0",
      73, "+4c1+1d1+1",
      74, "+4d0+1f1+1",
      75, "+4+1+1+4d1-2",
      76, "+4+1c0f1+1",
      77, "+4+1+5e1-2",
      78, "+4+1+1e0-1",
      79, "+4+7+5e1-2",
      80, "+4+1+1e0",
      81, "+4+7+5e1+1",
      82, "+4+1+1e0+1",
      83, "+4+1+5e1+1",
      84, "+4+7+1e0+1",
      85, "+4+1+1+4d1+1",
      86, "+4+1+5+0e1+1",
      87, "+4+1+1+5e0+1",
      88, "+4+1c1+1d0",
      89, "+4+1+7+5f0-1",
      90, "+4+1+4f0-1",
      91, "+4+1+5f0-1",
      92, "+4+1+7f0-1",
      93, "+4+1+4e1-2",
      94, "+4+1f0-1",
      95, "+4+5f1-2",
      96, "+4+1f0",
      97, "+4+5f1+1",
      98, "+4+1f0+1",
      99, "+4+1+4e1+1",
     100, "+4+1+7f0+1",
     101, "+4+1+5f0+1",
     102, "+4+1+4f0+1",
     103, "+4+1+7+5f0+1",
     104, "+4+1c1f0+1",
     105, "+4+1+4+1e1+1",
     106, "+4+1+5+7f0+1",
     107, "+4+1+1+5e1-2",
     108, "+4+1+0+4e0+1",
     109, "+4+1+7+5e1-2",
     110, "+4e0+1g1-2",
     111, "+4c1-6e1-2",
     112, "+4c1-2e0",
     113, "+4c1-5e1-1",
     114, "+4e0-1d0+1",
     115, "+4+1+7+5e1+1",
     116, "+4+1c0g1-2",
     117, "+4+1+1+5e1+1",
     118, "+4d0+1g1-2",
     119, "+4c1+1e1-2",
     120, "+4c1h0-1",
     121, "+4c1-1e1+1",
     122, "+4+1+0g1-2",
     123, "+4+1+1g1-2",
     124, "+4+3g1-2",
     125, "+4+1g1-2",
     126, "+4h0-1",
     127, "+4g1-2",
     129, "+4g1+1",
     130, "+4h0+1",
     131, "+4+1g1+1",
     132, "+4+3g1+1",
     133, "+4+1+1g1+1",
     134, "+4+1+0g1+1",
     135, "+4c1-1e1-2",
     136, "+4c1h0+1",
     137, "+4c1+1e1+1",
     138, "+4d0+1g1+1",
     139, "+4+1c1+1e1+1",
     140, "+4+1c0g1+1",
     141, "+4+1+4+5e1-2",
     142, "+4e0+1d0-1",
     143, "+4c1+5e1-2",
     144, "+4c1+1e0",
     145, "+4c1+5e1+1",
     146, "+4e0+1d0+1",
     147, "+4+1+4+5e1+1",
     148, "+4+1+1c0g1+1",
     149, "+4+1d1+5d1-2",
     150, "+4+1+1+4f0-1",
     151, "+4+5d1-5h0-1",
     152, "+4+1d0g1+1",
     153, "+4+1+1+5f0-1",
     154, "+4+1+5+0f1-2",
     155, "+4+1+1+4e1-2",
     156, "+4+7+1f0-1",
     157, "+4+1+5f1-2",
     158, "+4+1+1f0-1",
     159, "+4+7+5f1-2",
     160, "+4+1+1f0",
     161, "+4+7+5f1+1",
     162, "+4+1+1f0+1",
     163, "+4+1+5f1+1",
     164, "+4+7+1f0+1",
     165, "+4+1+1+4e1+1",
     166, "+4+1+5+0f1+1",
     167, "+4+1+1+5f0+1",
     168, "+4d0+5e1+1",
     169, "+4+5d1-1d1+1",
     170, "+4+1+1+4f0+1",
     171, "+4+1+1d1-5f0-1",
     172, "+4+1+5c0f1+1",
     173, "+4+1c1+5e1-2",
     174, "+4+1d0-1d0-1",
     175, "+4+5c1-6e1-2",
     176, "+4+1c1+1e0",
     177, "+4+5c1-5e1-1",
     178, "+4+1c0+5e0-1",
     179, "+4+1c1+5e1+1",
     180, "+4+1+0+4f0-1",
     181, "+4+1c1+5g0-1",
     182, "+4+1+5+7g0-1",
     183, "+4+1+4+1f1-2",
     184, "+4+1c1g0-1",
     185, "+4+1+7+5g0-1",
     186, "+4+1+4g0-1",
     187, "+4+1+5g0-1",
     188, "+4+1+7g0-1",
     189, "+4+1+4f1-2",
     190, "+4+1g0-1",
     191, "+4+5g1-2",
     192, "+4+1g0",
     193, "+4+5g1+1",
     194, "+4+1g0+1",
     195, "+4+1+4f1+1",
     196, "+4+1+7g0+1",
     197, "+4+1+5g0+1",
     198, "+4+1+4g0+1",
     199, "+4+1+7+5g0+1",
     200, "+4+1c1g0+1",
     210, "+4+1d0+1d0+1",
     220, "+4+1+7+1f0-1",
     230, "+4+1d0+1h1-2",
     240, "+4d1-2e0",
     250, "+4+1+0h1-2",
     260, "+4+3h1+1",
     270, "+4e0-1h1+1",
     280, "+4+1d0h1+1",
     290, "+4e0+1e0+1",
     300, "+4d0+1+4f0-1",
     310, "+4+1+1+4g0-1",
     320, "+4+1+1g0",
     330, "+4+1+1+4g0+1",
     340, "+4+5c1-2f0-1",
     350, "+4+1d0-1e0-1",
     360, "+4+1+4c1h0-1",
     370, "+4+1c0+5f0-1",
     380, "+4+1+7h0-1",
     390, "+4+1+4h0+1",
     400, "+4+1d1h0+1",
     410, "+4+1d0+5e0+1",
     420, "+4e0-1+4f0-1",
     430, "+4+1e0-5d0+1",
     440, "+4c1-2g0-1",
     450, "+4e0-1f0+1",
     460, "+4+1c0+5f1+1",
     470, "+4+1+0f1-5d1-1",
     480, "+4d1-2f0",
     490, "+4+1d0-1i1-2",
     500, "+4+1c0i1-2",
     510, "+4j0-1",
     520, "+4c1j0+1",
     530, "+4e0+1i1+1",
     540, "+4+3e1-1e1-2",
     550, "+4+1+0e1+1e1+1",
     560, "+4+1e0i1+1",
     570, "+4+1+4+1g0-1",
     580, "+4+1g0+5+1+1",
     590, "+4+1+1e0-1i1+1",
     600, "+4+1f1+1d0+1",
     610, "+4+1f0+1i1+1",
     620, "+4d0+1+4g0-1",
     630, "+4+1+1+4h0-1",
     640, "+4+1+1h0",
     650, "+4+1+1+4h0+1",
     660, "+4d0+1+4g0+1",
     670, "+4+1f0+5d0-1",
     680, "+4+1d0-5f1-2",
     690, "+4+1c1+5+1g1-2",
     700, "+4+1+7+1+1g0-1",
     710, "+4+1+0d1+5f1+1",
     720, "+4+1+4d1-2e0",
     740, "+4+1+7+5c1i0-1",
     750, "+4+1f0-1d0-1",
     760, "+4+1c1i0-1",
     770, "+4+1i0+1",
     780, "+4+1+0+4h0+1",
     790, "+4+1d0-5f0+1",
     800, "+4+1e0+1e0",
     820, "+4+5c1+1g0-1",
     830, "+4+1d0+1f0-1",
     840, "+4+1+1c1+1g0+1",
     850, "+4+1e0+1+4d1+1",
     860, "+4+1+7f0-5d0+1",
     880, "+4e0-5g1-2",
     890, "+4+5+7+1h0-1",
     900, "+4+1+7+1h0+1",
     910, "+4c1-2+4h0+1",
     920, "+4+1g1+1d0-1",
     930, "+4+1f0-1j1-2",
     940, "+4+1+5g0-5c1+1",
     950, "+4+1+1+4+1g0-1",
     960, "+4d1-2g0",
     970, "+4+1+0g1-6d1-2",
     980, "+4+1+5g0+5c1+1",
     990, "+4f0+1j1-2",
    1000, "+4+1d0j1-2",
    1050, "+4+1d0+1j1+1",
    1100, "+4+1+5c0+5g1+1",
    1150, "+4e0+1g0-1",
    1200, "+4+1d1+1g0-1",
    1300, "+4d0+1+4h0+1",
    1400, "+4+1c1+1h0-1",
    1500, "+4+1+4+1c0i1-2",
    1550, "+4+1c0+5h0+1",
    1600, "+4+1e0+1f0",
    1650, "+4+1c0+5+1g0-1",
    1700, "+4+1e0+1+4f0+1",
    1800, "+4c1-1i0-2",
    1900, "+4+1+5h0-5c1+1",
    1950, "+4+1f0+1k1-2",
    2000, "+4+1e0k1-2",
    2050, "+4l0+1",
    2100, "+4+1+7e0+1j1+1",
    2150, "+4+1+0g1+5e1+1",
    2200, "+4+1d0g1+1e1+1",
    2300, "+4+1i0-5+1+1",
    2400, "+4+1+0e1+1g0-1",
    2450, "+4c1+5+7+1h0+1",
    2500, "+4+7+1f0-1e0+1",
    2550, "+4+1+1+4j0-1",
    2600, "+4+1+1+4c1j0+1",
    2800, "+4+1+0d1+1h0-1",
    2850, "+4+1e0-5+1g1-2",
    3050, "+4+1d0-5h0-1",
    3100, "+4+1+7+5c1k0+1",
    3150, "+4+1e0+1+4f1-2",
    3200, "+4+1e0+1g0",
    3250, "+4+1e0+1+4f1+1",
    3300, "+4+1d0+1+5h0-1",
    3600, "+4e0-5i1+1",
    3800, "+4c1+1f0+1j1-2",
    3850, "+4+1+1+4+1i0+1",
    3900, "+4f0-1+4h0+1",
    4000, "+4+1f0l1-2",
    4050, "+4+1e0-1l1-2",
    4100, "+4+3l1+1",
    4200, "+4+1c1f0+1j1+1",
    4300, "+4+1c0h1+5e1+1",
    4350, "+4f0+1h0-1",
    4400, "+4+1h1+1e0+1",
    4600, "+4c1+1j0-1",
    4700, "+4c1+5c1+1h0-1",
    4800, "+4+1+1+4d1-2g0",
    5000, "+4c1-1h0-1j1-2",
    5100, "+4d0+1+4j0-1",
    5200, "+4+1+1+4d1k0+1",
    5400, "+4+1+4c1-1i0-2",
    5700, "+4+1g0-1+4f0-1",
    5950, "+4+1g0+5f0-1",
    6000, "+4+1+4+1e0k1-2",
    6050, "+4+1f0-5g0-1",
    6100, "+4+1+7e0-5h0-1",
    6150, "+4+1+4l0+1",
    6200, "+4+1+7+5d1l0+1",
    6300, "+4+1e0+1+4h0-1",
    6350, "+4+1e0+1+4g1-2",
    6400, "+4+1e0+1h0",
    6450, "+4+1e0+1+4g1+1",
    6500, "+4+1e0+1+4h0+1",
    6650, "+4+1+0e1-6i1-2",
    7200, "+4+3e1-1i0-2",
    7400, "+4+1+5d0+5i1-2",
    7600, "+4+1+1i1-2e0-1",
    7650, "+4d1-1+4j0-2",
    7700, "+4+1+5j0+5c1+1",
    7800, "+4+1c1h0+1k1-2",
    8000, "+4+1g0m1-2",
    8100, "+4+1+7f0-1l1-2",
    8200, "+4c1n0+1",
    8350, "+4+1+1f0-1m1+1",
    8400, "+4+1d0+1d0m1+1",
    8450, "+4g0+1h0+1",
    8700, "+4+3f1+5h1-2",
    8900, "+4d1+5c1+1h0+1",
    9200, "+4e0+5j1-2",
    9250, "+4e0+5+1j1+1",
    9600, "+4+1+1+4d1-2h0",
    9800, "+4c1+1g0+5e0+1",
};

int	multabsize = sizeof(multab) / sizeof(multab[0]);
