# Shell Lab實驗紀錄

## Intro

Files:

- `Makefile` &rarr; Compiles your shell program and runs the tests
- `README.md`  &rarr; This file
- `tsh.c` &rarr; The shell program that you will write and hand in
- `tshref` &rarr; The reference shell binary.

### The remaining files are used to test your shell
- `sdriver.pl` &rarr; The trace-driven shell driver
- `trace*.txt` &rarr; The 15 trace files that control the shell driver
- `tshref.out`  &rarr;  Example output of the reference shell on all 15 traces

### Little C programs that are called by the trace files
- `myspin.c` &rarr; Takes argument <n> and spins for <n> seconds
- `mysplit.c` &rarr; Forks a child that spins for <n> seconds
- `mystop.c ` &rarr; Spins for <n> seconds and sends SIGTSTP to itself
- `myint.c` &rarr; Spins for <n> seconds and sends SIGINT to itself

### 實驗要求
- 補全tsh.c中剩餘的代碼：
- void eval(char *cmdline)：解析並執行命令。
- int builtin_cmd(char **argv)：檢測命令是否為內置命令quit、fg、bg、jobs。
- void do_bgfg(char **argv)：實現bg、fg命令。
- void waitfg(pid_t pid)：等待前台命令執行完成。
- void sigchld_handler(int sig)：處理SIGCHLD信號，即子進程停止或終止。
- void sigint_handler(int sig)：處理SIGINT信號，即來自鍵盤的中斷ctrl-c。
- void sigtstp_handler(int sig)：處理SIGTSTP信號，即終端停止信號ctrl-z。
 
  使用make testn用來測試你編寫的shell執行第n組測試數據的輸出。- 

  使用make rtestn用來測試參考shell程序第n組測試數據的輸出（共16組測試數據）。- 

  tshref.out包含參考shell程序的所有測試數據的輸出結果，先看完該文件了解命令格式在開始編碼。

### 可用輔助函數：
- int parseline(const char *cmdline,char **argv)：獲取參數列表char **argv，返回是否為後台運行命令（true）。
- void clearjob(struct job_t *job)：清除job結構。
- void initjobs(struct job_t *jobs)：初始化jobs鍊錶。
- void maxjid(struct job_t *jobs)：返回jobs鍊錶中最大的jid號。
- int addjob(struct job_t *jobs,pid_t pid,int state,char *cmdline)：在jobs鍊錶中添加job
- int deletejob(struct job_t *jobs,pid_t pid)：在jobs鍊錶中刪除pid的job。
- pid_t fgpid(struct job_t *jobs)：返回當前前台運行job的pid號。
- struct job_t *getjobpid(struct job_t *jobs,pid_t pid)：返回pid號的job。
- struct job_t *getjobjid(struct job_t *jobs,int jid)：返回jid號的job。
- int pid2jid(pid_t pid)：將pid號轉化為jid。
- void listjobs(struct job_t *jobs)：打印jobs。
- void sigquit_handler(int sig)：處理SIGQUIT信號。
- 上述文字複製Pipapa的知乎文章，知乎好像不能完美支持latex，但是複制卻可以……

### 注意事項
- tsh的提示符為tsh>
- 用戶的輸入分為第一個的name和後面的參數，之間以一個或多個空格隔開。如果name是一個tsh內置的命令，那麼tsh應該馬上處理這個命令然後等待下一個輸入。否則，tsh應該假設name是一個路徑上的可執行文件，並在一個子進程中運行這個文件（這也稱為一個工作、job）
- tsh不需要支持管道和重定向
- 如果用戶輸入ctrl-c( ctrl-z)，那麼SIGINT( SIGTSTP)信號應該被送給每一個在前台進程組中的進程，如果沒有進程，那麼這兩個信號應該不起作用。
- 如果一個命令以“&”結尾，那麼tsh應該將它們放在後台運行，否則就放在前台運行（並等待它的結束）
- 每一個工作（job）都有一個正整數PID或者job ID（JID）。JID通過"%"前綴標識符表示，例如，“%5”表示JID為5的工作，而“5”代筆PID為5的進程。

- tsh應該有如下內置命令：

    quit: 退出当前shell

    jobs: 列出所有后台运行的工作

    bg <job>: 这个命令将会向<job>代表的工作发送SIGCONT信号并放在后台运行，<job>可以是一个PID也可以是一个JID。

    fg <job>: 这个命令会向<job>代表的工作发送SIGCONT信号并放在前台运行，<job>可以是一个PID也可以是一个JID。

- tsh應該回收（reap）所有殭屍孩子，如果一個工作是因為收到了一個它沒有捕獲的（沒有按照信號處理函數）而終止的，那麼tsh應該輸出這個工作的PID和這個信號的相關描述。

### 提示
- 仔細閱讀CSAPP第八章的異常控制流和lab的writeup- 

- make testn測試shell執行第n組測試數據的輸出，make rtestn打印shell預期輸出，tshref.out包含shell所有預期輸出結果，先看文件輸出，了解命令格式再編碼，修改makefile文件中CFLAGS字段，加-g參數並- 去掉-O2參數- 

- waitpid, kill, fork, execve, setpgid,sigprocmask很常用，可通過命令手冊查看使用細節，WUNTRACED和WNOHANG選項對waitpid也很有用- 

- 實現信息處理函數，確保發送SIGINT和SIGTSTP信號給整個前台進程組，用-pid代替pid作為kill參數- 

- 建議在waitfg的循環中用sleep函數，在sigchld_handler中對waitpid只調用一次- 

- eval中進程在fork之前用sigprocmask阻塞SIGCHLD信號，之後在解除信號阻塞，之後在調用addjob添加孩子到作業列表用sigprocmask阻塞信號，因為子繼承繼承父進程的阻塞集合，所以子程序必須確保在執行新進- 程前解除阻塞SIGCHLD信號。父進程需以這種方式阻塞SIGCHLD信號，避免在父進程調用addjob之前，SIGCHLD處理器獲取子進程(從而從任務列表中刪除)的競爭狀態。- 

- 不要直接調用常用命令，而應輸入完整路徑，如/bin/ls- 

- 當在標準Unix shell運行tsh時，tsh運行在前台進程組中。若tsh隨後創建子進程，默認情況下，該子進程也是前台進程組的成員。因為按下ctrl-c會向前台組中的每個進程發送SIGINT信號，按下ctrl-c會向tsh及- Unix shell創建的每個子進程，顯然不正確。應該在fork後，但在execve前，子進程調用setpgid(0,0)，把子進程放到新進程組中，該進程組ID與子進程的PID相同。確保前台進程組中只有一個進程，即tsh進程。當- 按下ctrl-c時，tsh應捕獲生成的SIGINT，然後將其轉發給包含前台作業的進程組。