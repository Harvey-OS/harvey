#include "common.h"
#include "send.h"

/* configuration */
#define LOGBiobuf "log/status"

/* log mail delivery */
extern void
logdelivery(dest *list, char *rcvr, message *mp)
{
	dest *parent;

	for(parent=list; parent->parent!=0; parent=parent->parent)
		;
	if(parent!=list && strcmp(s_to_c(parent->addr), rcvr)!=0)
		syslog(0, "mail", "delivered %s From %.256s %.256s (%.256s) %d",
			rcvr,
			s_to_c(mp->sender), s_to_c(mp->date),
			s_to_c(parent->addr), mp->size);
	else
		syslog(0, "mail", "delivered %s From %.256s %.256s %d", rcvr,
			s_to_c(mp->sender), s_to_c(mp->date), mp->size);
}

/* log mail forwarding */
extern void
loglist(dest *list, message *mp, char *tag)
{
	dest *next;
	dest *parent;

	for(next=d_rm(&list); next != 0; next = d_rm(&list)) {
		for(parent=next; parent->parent!=0; parent=parent->parent)
			;
		if(parent!=next)
			syslog(0, "mail", "%s %.256s From %.256s %.256s (%.256s) %d",
				tag,
				s_to_c(next->addr), s_to_c(mp->sender),
				s_to_c(mp->date), s_to_c(parent->addr), mp->size);
		else
			syslog(0, "mail", "%s %.256s From %.256s %.256s %d\n", tag,
				s_to_c(next->addr), s_to_c(mp->sender),
				s_to_c(mp->date), mp->size);
	}
}

/* log a mail refusal */
extern void
logrefusal(dest *dp, message *mp, char *msg)
{
	char buf[2048];
	char *cp, *ep;

	sprint(buf, "error %.256s From %.256s %.256s\nerror+ ", s_to_c(dp->addr),
		s_to_c(mp->sender), s_to_c(mp->date));
	cp = buf + strlen(buf);
	ep = buf + sizeof(buf) - sizeof("error + ");
	while(*msg && cp<ep) {
		*cp++ = *msg;
		if (*msg++ == '\n') {
			strcpy(cp, "error+ ");
			cp += sizeof("error+ ") - 1;
		}
	}
	*cp = 0;
	syslog(0, "mail", "%s", buf);
}
