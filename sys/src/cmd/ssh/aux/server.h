
/* Used by the server */
extern RSApub	*client_session_key;
extern RSApub	*client_host_key;
extern char	 client_host_name[];

extern char	user[16];

extern RSApriv	*server_session_key;
extern RSApriv	*server_host_key;

extern char	*syslogfile;

extern uchar	server_cookie[8];
extern ulong	protocol_flags;
extern ulong	supported_cipher_mask;
extern ulong	supported_authentications_mask;

void		put_ssh_smsg_public_key(void);
void		get_ssh_cmsg_session_key(void);
void		put_ssh_smsg_success(void);
void		put_ssh_smsg_failure(void);
void		get_ssh_cmsg_user(void);
void		put_ssh_smsg_exitstatus(long s);
int		ssh_smsg_auth_rsa_challenge(void);
void		user_auth(void);
void		start_work(void);
void		run_cmd(char *);
void		syserror(char *, ...);
