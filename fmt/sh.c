8550 // Shell.
8551 
8552 #include "types.h"
8553 #include "user.h"
8554 #include "fcntl.h"
8555 
8556 // Parsed command representation
8557 #define EXEC  1
8558 #define REDIR 2
8559 #define PIPE  3
8560 #define LIST  4
8561 #define BACK  5
8562 
8563 #define MAXARGS 10
8564 
8565 struct cmd {
8566   int type;
8567 };
8568 
8569 struct execcmd {
8570   int type;
8571   char *argv[MAXARGS];
8572   char *eargv[MAXARGS];
8573 };
8574 
8575 struct redircmd {
8576   int type;
8577   struct cmd *cmd;
8578   char *file;
8579   char *efile;
8580   int mode;
8581   int fd;
8582 };
8583 
8584 struct pipecmd {
8585   int type;
8586   struct cmd *left;
8587   struct cmd *right;
8588 };
8589 
8590 struct listcmd {
8591   int type;
8592   struct cmd *left;
8593   struct cmd *right;
8594 };
8595 
8596 struct backcmd {
8597   int type;
8598   struct cmd *cmd;
8599 };
8600 int fork1(void);  // Fork but panics on failure.
8601 void panic(char*);
8602 struct cmd *parsecmd(char*);
8603 
8604 // Execute cmd.  Never returns.
8605 void
8606 runcmd(struct cmd *cmd)
8607 {
8608   int p[2];
8609   struct backcmd *bcmd;
8610   struct execcmd *ecmd;
8611   struct listcmd *lcmd;
8612   struct pipecmd *pcmd;
8613   struct redircmd *rcmd;
8614 
8615   if(cmd == 0)
8616     exit();
8617 
8618   switch(cmd->type){
8619   default:
8620     panic("runcmd");
8621 
8622   case EXEC:
8623     ecmd = (struct execcmd*)cmd;
8624     if(ecmd->argv[0] == 0)
8625       exit();
8626     exec(ecmd->argv[0], ecmd->argv);
8627     printf(2, "exec %s failed\n", ecmd->argv[0]);
8628     break;
8629 
8630   case REDIR:
8631     rcmd = (struct redircmd*)cmd;
8632     close(rcmd->fd);
8633     if(open(rcmd->file, rcmd->mode) < 0){
8634       printf(2, "open %s failed\n", rcmd->file);
8635       exit();
8636     }
8637     runcmd(rcmd->cmd);
8638     break;
8639 
8640   case LIST:
8641     lcmd = (struct listcmd*)cmd;
8642     if(fork1() == 0)
8643       runcmd(lcmd->left);
8644     wait();
8645     runcmd(lcmd->right);
8646     break;
8647 
8648 
8649 
8650   case PIPE:
8651     pcmd = (struct pipecmd*)cmd;
8652     if(pipe(p) < 0)
8653       panic("pipe");
8654     if(fork1() == 0){
8655       close(1);
8656       dup(p[1]);
8657       close(p[0]);
8658       close(p[1]);
8659       runcmd(pcmd->left);
8660     }
8661     if(fork1() == 0){
8662       close(0);
8663       dup(p[0]);
8664       close(p[0]);
8665       close(p[1]);
8666       runcmd(pcmd->right);
8667     }
8668     close(p[0]);
8669     close(p[1]);
8670     wait();
8671     wait();
8672     break;
8673 
8674   case BACK:
8675     bcmd = (struct backcmd*)cmd;
8676     if(fork1() == 0)
8677       runcmd(bcmd->cmd);
8678     break;
8679   }
8680   exit();
8681 }
8682 
8683 int
8684 getcmd(char *buf, int nbuf)
8685 {
8686   printf(2, "$ ");
8687   memset(buf, 0, nbuf);
8688   gets(buf, nbuf);
8689   if(buf[0] == 0) // EOF
8690     return -1;
8691   return 0;
8692 }
8693 
8694 
8695 
8696 
8697 
8698 
8699 
8700 int
8701 main(void)
8702 {
8703   static char buf[100];
8704   int fd;
8705 
8706   // Ensure that three file descriptors are open.
8707   while((fd = open("console", O_RDWR)) >= 0){
8708     if(fd >= 3){
8709       close(fd);
8710       break;
8711     }
8712   }
8713 
8714   // Read and run input commands.
8715   while(getcmd(buf, sizeof(buf)) >= 0){
8716     if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
8717       // Chdir must be called by the parent, not the child.
8718       buf[strlen(buf)-1] = 0;  // chop \n
8719       if(chdir(buf+3) < 0)
8720         printf(2, "cannot cd %s\n", buf+3);
8721       continue;
8722     }
8723     if(fork1() == 0)
8724       runcmd(parsecmd(buf));
8725     wait();
8726   }
8727   exit();
8728 }
8729 
8730 void
8731 panic(char *s)
8732 {
8733   printf(2, "%s\n", s);
8734   exit();
8735 }
8736 
8737 int
8738 fork1(void)
8739 {
8740   int pid;
8741 
8742   pid = fork();
8743   if(pid == -1)
8744     panic("fork");
8745   return pid;
8746 }
8747 
8748 
8749 
8750 // Constructors
8751 
8752 struct cmd*
8753 execcmd(void)
8754 {
8755   struct execcmd *cmd;
8756 
8757   cmd = malloc(sizeof(*cmd));
8758   memset(cmd, 0, sizeof(*cmd));
8759   cmd->type = EXEC;
8760   return (struct cmd*)cmd;
8761 }
8762 
8763 struct cmd*
8764 redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
8765 {
8766   struct redircmd *cmd;
8767 
8768   cmd = malloc(sizeof(*cmd));
8769   memset(cmd, 0, sizeof(*cmd));
8770   cmd->type = REDIR;
8771   cmd->cmd = subcmd;
8772   cmd->file = file;
8773   cmd->efile = efile;
8774   cmd->mode = mode;
8775   cmd->fd = fd;
8776   return (struct cmd*)cmd;
8777 }
8778 
8779 struct cmd*
8780 pipecmd(struct cmd *left, struct cmd *right)
8781 {
8782   struct pipecmd *cmd;
8783 
8784   cmd = malloc(sizeof(*cmd));
8785   memset(cmd, 0, sizeof(*cmd));
8786   cmd->type = PIPE;
8787   cmd->left = left;
8788   cmd->right = right;
8789   return (struct cmd*)cmd;
8790 }
8791 
8792 
8793 
8794 
8795 
8796 
8797 
8798 
8799 
8800 struct cmd*
8801 listcmd(struct cmd *left, struct cmd *right)
8802 {
8803   struct listcmd *cmd;
8804 
8805   cmd = malloc(sizeof(*cmd));
8806   memset(cmd, 0, sizeof(*cmd));
8807   cmd->type = LIST;
8808   cmd->left = left;
8809   cmd->right = right;
8810   return (struct cmd*)cmd;
8811 }
8812 
8813 struct cmd*
8814 backcmd(struct cmd *subcmd)
8815 {
8816   struct backcmd *cmd;
8817 
8818   cmd = malloc(sizeof(*cmd));
8819   memset(cmd, 0, sizeof(*cmd));
8820   cmd->type = BACK;
8821   cmd->cmd = subcmd;
8822   return (struct cmd*)cmd;
8823 }
8824 
8825 
8826 
8827 
8828 
8829 
8830 
8831 
8832 
8833 
8834 
8835 
8836 
8837 
8838 
8839 
8840 
8841 
8842 
8843 
8844 
8845 
8846 
8847 
8848 
8849 
8850 // Parsing
8851 
8852 char whitespace[] = " \t\r\n\v";
8853 char symbols[] = "<|>&;()";
8854 
8855 int
8856 gettoken(char **ps, char *es, char **q, char **eq)
8857 {
8858   char *s;
8859   int ret;
8860 
8861   s = *ps;
8862   while(s < es && strchr(whitespace, *s))
8863     s++;
8864   if(q)
8865     *q = s;
8866   ret = *s;
8867   switch(*s){
8868   case 0:
8869     break;
8870   case '|':
8871   case '(':
8872   case ')':
8873   case ';':
8874   case '&':
8875   case '<':
8876     s++;
8877     break;
8878   case '>':
8879     s++;
8880     if(*s == '>'){
8881       ret = '+';
8882       s++;
8883     }
8884     break;
8885   default:
8886     ret = 'a';
8887     while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
8888       s++;
8889     break;
8890   }
8891   if(eq)
8892     *eq = s;
8893 
8894   while(s < es && strchr(whitespace, *s))
8895     s++;
8896   *ps = s;
8897   return ret;
8898 }
8899 
8900 int
8901 peek(char **ps, char *es, char *toks)
8902 {
8903   char *s;
8904 
8905   s = *ps;
8906   while(s < es && strchr(whitespace, *s))
8907     s++;
8908   *ps = s;
8909   return *s && strchr(toks, *s);
8910 }
8911 
8912 struct cmd *parseline(char**, char*);
8913 struct cmd *parsepipe(char**, char*);
8914 struct cmd *parseexec(char**, char*);
8915 struct cmd *nulterminate(struct cmd*);
8916 
8917 struct cmd*
8918 parsecmd(char *s)
8919 {
8920   char *es;
8921   struct cmd *cmd;
8922 
8923   es = s + strlen(s);
8924   cmd = parseline(&s, es);
8925   peek(&s, es, "");
8926   if(s != es){
8927     printf(2, "leftovers: %s\n", s);
8928     panic("syntax");
8929   }
8930   nulterminate(cmd);
8931   return cmd;
8932 }
8933 
8934 struct cmd*
8935 parseline(char **ps, char *es)
8936 {
8937   struct cmd *cmd;
8938 
8939   cmd = parsepipe(ps, es);
8940   while(peek(ps, es, "&")){
8941     gettoken(ps, es, 0, 0);
8942     cmd = backcmd(cmd);
8943   }
8944   if(peek(ps, es, ";")){
8945     gettoken(ps, es, 0, 0);
8946     cmd = listcmd(cmd, parseline(ps, es));
8947   }
8948   return cmd;
8949 }
8950 struct cmd*
8951 parsepipe(char **ps, char *es)
8952 {
8953   struct cmd *cmd;
8954 
8955   cmd = parseexec(ps, es);
8956   if(peek(ps, es, "|")){
8957     gettoken(ps, es, 0, 0);
8958     cmd = pipecmd(cmd, parsepipe(ps, es));
8959   }
8960   return cmd;
8961 }
8962 
8963 struct cmd*
8964 parseredirs(struct cmd *cmd, char **ps, char *es)
8965 {
8966   int tok;
8967   char *q, *eq;
8968 
8969   while(peek(ps, es, "<>")){
8970     tok = gettoken(ps, es, 0, 0);
8971     if(gettoken(ps, es, &q, &eq) != 'a')
8972       panic("missing file for redirection");
8973     switch(tok){
8974     case '<':
8975       cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
8976       break;
8977     case '>':
8978       cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
8979       break;
8980     case '+':  // >>
8981       cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
8982       break;
8983     }
8984   }
8985   return cmd;
8986 }
8987 
8988 
8989 
8990 
8991 
8992 
8993 
8994 
8995 
8996 
8997 
8998 
8999 
9000 struct cmd*
9001 parseblock(char **ps, char *es)
9002 {
9003   struct cmd *cmd;
9004 
9005   if(!peek(ps, es, "("))
9006     panic("parseblock");
9007   gettoken(ps, es, 0, 0);
9008   cmd = parseline(ps, es);
9009   if(!peek(ps, es, ")"))
9010     panic("syntax - missing )");
9011   gettoken(ps, es, 0, 0);
9012   cmd = parseredirs(cmd, ps, es);
9013   return cmd;
9014 }
9015 
9016 struct cmd*
9017 parseexec(char **ps, char *es)
9018 {
9019   char *q, *eq;
9020   int tok, argc;
9021   struct execcmd *cmd;
9022   struct cmd *ret;
9023 
9024   if(peek(ps, es, "("))
9025     return parseblock(ps, es);
9026 
9027   ret = execcmd();
9028   cmd = (struct execcmd*)ret;
9029 
9030   argc = 0;
9031   ret = parseredirs(ret, ps, es);
9032   while(!peek(ps, es, "|)&;")){
9033     if((tok=gettoken(ps, es, &q, &eq)) == 0)
9034       break;
9035     if(tok != 'a')
9036       panic("syntax");
9037     cmd->argv[argc] = q;
9038     cmd->eargv[argc] = eq;
9039     argc++;
9040     if(argc >= MAXARGS)
9041       panic("too many args");
9042     ret = parseredirs(ret, ps, es);
9043   }
9044   cmd->argv[argc] = 0;
9045   cmd->eargv[argc] = 0;
9046   return ret;
9047 }
9048 
9049 
9050 // NUL-terminate all the counted strings.
9051 struct cmd*
9052 nulterminate(struct cmd *cmd)
9053 {
9054   int i;
9055   struct backcmd *bcmd;
9056   struct execcmd *ecmd;
9057   struct listcmd *lcmd;
9058   struct pipecmd *pcmd;
9059   struct redircmd *rcmd;
9060 
9061   if(cmd == 0)
9062     return 0;
9063 
9064   switch(cmd->type){
9065   case EXEC:
9066     ecmd = (struct execcmd*)cmd;
9067     for(i=0; ecmd->argv[i]; i++)
9068       *ecmd->eargv[i] = 0;
9069     break;
9070 
9071   case REDIR:
9072     rcmd = (struct redircmd*)cmd;
9073     nulterminate(rcmd->cmd);
9074     *rcmd->efile = 0;
9075     break;
9076 
9077   case PIPE:
9078     pcmd = (struct pipecmd*)cmd;
9079     nulterminate(pcmd->left);
9080     nulterminate(pcmd->right);
9081     break;
9082 
9083   case LIST:
9084     lcmd = (struct listcmd*)cmd;
9085     nulterminate(lcmd->left);
9086     nulterminate(lcmd->right);
9087     break;
9088 
9089   case BACK:
9090     bcmd = (struct backcmd*)cmd;
9091     nulterminate(bcmd->cmd);
9092     break;
9093   }
9094   return cmd;
9095 }
9096 
9097 
9098 
9099 
