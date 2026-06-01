#include <iostream>
// lazygitcpp - C++ git TUI (port of lazygit core workflow)
// Uses system git, renders with ANSI escape codes

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>

static const char* VERSION = "0.1.0";
static std::atomic<bool> running(true);
static struct termios origTerm;

static const char *R="\033[0m",*B="\033[1m",*D="\033[2m",*C="\033[36m",*M="\033[35m",*Y="\033[33m",*G="\033[32m",*RD="\033[31m",*W="\033[37m";

static void enableRaw() { struct termios t; tcgetattr(STDIN_FILENO, &t); origTerm=t; t.c_lflag&=~(ECHO|ICANON); t.c_cc[VMIN]=0;t.c_cc[VTIME]=1;tcsetattr(STDIN_FILENO,TCSANOW,&t); }
static void disableRaw() { tcsetattr(STDIN_FILENO,TCSANOW,&origTerm); }
static int tW() { struct winsize ws; ioctl(0,TIOCGWINSZ,&ws); return ws.ws_col>0?ws.ws_col:80; }
static int tH() { struct winsize ws; ioctl(0,TIOCGWINSZ,&ws); return ws.ws_row>0?ws.ws_row:24; }

static std::string git(const std::string& args) {
    std::string cmd = "git " + args + " 2>&1";
    FILE* f = popen(cmd.c_str(), "r");
    if (!f) return ""; char buf[4096]; std::string o;
    while (fgets(buf,sizeof(buf),f)) o += buf;
    pclose(f); while (!o.empty()&&o.back()=='\n') o.pop_back();
    return o;
}

static std::vector<std::string> gitLines(const std::string& args) {
    std::string out = git(args);
    std::vector<std::string> lines; std::istringstream ss(out); std::string l;
    while (std::getline(ss,l)) { while(!l.empty()&&l.back()=='\r')l.pop_back(); if(!l.empty())lines.push_back(l); }
    return lines;
}

enum Panel { STATUS=0, BRANCHES=1, LOG=2, STASH=3, PANEL_COUNT=4 };
static Panel curPanel = STATUS;
static int selStatus=0, selBranch=0, selLog=0, selStash=0;
static std::string commitMsg, lastError;

/* Get status: staged + unstaged files */
struct FileEntry { std::string status, file; };
static std::vector<FileEntry> getFiles() {
    std::vector<FileEntry> r;
    auto lines = gitLines("status --porcelain");
    for (auto& l : lines) {
        if (l.size() >= 3) r.push_back({l.substr(0,2), l.substr(3)});
    }
    return r;
}

/* Get branches */
static std::vector<std::string> getBranches() {
    return gitLines("branch --sort=-committerdate");
}

/* Get log */
static std::vector<std::string> getLog() {
    return gitLines("log --oneline --graph -30");
}

/* Get stashes */
static std::vector<std::string> getStash() {
    return gitLines("stash list");
}

static std::string getBranchName() {
    return git("rev-parse --abbrev-ref HEAD");
}

/* Render a panel with title + lines + scrollbar */
static void renderPanel(int x, int y, int w, int h, const std::string& title,
                        const std::vector<std::string>& lines, int sel, bool focused,
                        const char* titleColor) {
    /* Title bar */
    printf("\033[%d;%dH", y, x);
    if (focused) printf("\033[7m");
    printf("%s %-*s %s", titleColor, w-2, title.c_str(), R);
    if (focused) printf("\033[0m");

    /* Content */
    int start = 0;
    if (sel >= h - 3) start = sel - (h - 4);
    if (start < 0) start = 0;
    int shown = 0;
    for (int i = start; i < (int)lines.size() && shown < h - 2; i++, shown++) {
        printf("\033[%d;%dH", y + 1 + shown, x);
        if (i == sel && focused) printf("\033[7m");
        std::string l = lines[i];
        if ((int)l.size() > w - 2) l = l.substr(0, w - 2);
        printf("%-*s", w, l.c_str());
        if (i == sel && focused) printf("\033[0m");
    }
    /* Clear remaining */
    for (int i = shown; i < h - 2; i++) {
        printf("\033[%d;%dH%-*s", y + 1 + i, x, w, "");
    }
}

/* Render full screen */
static void renderAll() {
    int w = tW(), h = tH();
    printf("\033[2J\033[H");

    auto files = getFiles();
    auto branches = getBranches();
    auto logs = getLog();
    auto stashes = getStash();
    std::string branch = getBranchName();

    /* Header */
    printf("%s%s lazygitcpp %s%s%s  %s[%s%s%s]%s  %s[1:status|2:branches|3:log|4:stash|s:stage|c:commit|p:push|P:pull|q:quit]%s\n",
           B, C, VERSION, R, D, Y, branch.c_str(), D, R, D, R);
    if (!lastError.empty()) { printf("%s%s%s\n", RD, lastError.c_str(), R); lastError.clear(); }

    /* Panel layout: 4 columns, 2x2 grid */
    int pw = w / 2 - 1, ph = (h - 3) / 2;
    int col1 = 0, col2 = pw + 1;
    int row1 = 2, row2 = row1 + ph + 1;

    /* Status */
    renderPanel(col1, row1, pw, ph, "Status (" + std::to_string(files.size()) + ")",
                [&](){ std::vector<std::string> ls; for(auto&f:files){
                    const char* c = (f.status[0]=='?'||f.status[0]=='A')?G:(f.status[0]=='M'||f.status[0]=='D')?Y:RD;
                    ls.push_back(std::string(c)+f.status+" "+f.file+R); } return ls; }(),
                selStatus, curPanel==STATUS, C);

    /* Branches */
    renderPanel(col2, row1, pw, ph, "Branches (" + std::to_string(branches.size()) + ")",
                branches, selBranch, curPanel==BRANCHES, M);

    /* Log */
    renderPanel(col1, row2, pw, ph, "Log (" + std::to_string(logs.size()) + ")",
                logs, selLog, curPanel==LOG, Y);

    /* Stash */
    renderPanel(col2, row2, pw, ph, "Stash (" + std::to_string(stashes.size()) + ")",
                stashes, selStash, curPanel==STASH, G);

    fflush(stdout);
}

/* Get selected file name (unstripped) */
static std::string selFile() {
    auto files = getFiles();
    if (selStatus >= 0 && selStatus < (int)files.size()) return files[selStatus].file;
    return "";
}

/* Stage/unstage file */
static void toggleStage() {
    std::string f = selFile();
    if (f.empty()) return;
    auto files = getFiles();
    if (selStatus >= (int)files.size()) return;
    std::string st = files[selStatus].status;
    /* If tracked modified, stage; if staged, unstage; if untracked, add */
    if (st[0] == '?' || st[0] == 'M' || st[1] == 'M') git("add '" + f + "'");
    else if (st[0] == 'A' || st[1] == 'M') git("reset HEAD '" + f + "'");
}

int main() {
    signal(SIGINT, [](int){ running=false; });
    enableRaw(); atexit(disableRaw);

    std::thread refresh([&](){ while(running){ renderAll(); std::this_thread::sleep_for(std::chrono::seconds(1)); } });

    while (running) {
        char c = getchar();
        int* sel = nullptr; int cnt = 0;
        switch (curPanel) {
            case STATUS: sel=&selStatus; cnt=getFiles().size(); break;
            case BRANCHES: sel=&selBranch; cnt=getBranches().size(); break;
            case LOG: sel=&selLog; cnt=getLog().size(); break;
            case STASH: sel=&selStash; cnt=getStash().size(); break;
            default: break;
        }

        if (c == 'q' || c == 27) running = false;
        else if (c == 'j' || c == 'J') { if (sel && *sel < cnt-1) (*sel)++; }
        else if (c == 'k' || c == 'K') { if (sel && *sel > 0) (*sel)--; }
        else if (c == '1') curPanel = STATUS;
        else if (c == '2') curPanel = BRANCHES;
        else if (c == '3') curPanel = LOG;
        else if (c == '4') curPanel = STASH;
        else if (c == ' ') { /* space: stage/unstage */
            if (curPanel == STATUS) toggleStage();
            else if (curPanel == BRANCHES) {
                auto branches = getBranches();
                if (selBranch >= 0 && selBranch < (int)branches.size()) {
                    std::string b = branches[selBranch];
                    /* Strip "* " prefix if present */
                    if (b[0] == '*') b = b.substr(2);
                    git("checkout '" + b + "'");
                }
            }
        }
        else if (c == 'c') { /* commit */
            if (curPanel == STATUS) {
                /* Read commit message via external editor or prompt */
                printf("\033[2J\033[HCommit message (type below, empty line to finish):\n");
                disableRaw();
                commitMsg.clear();
                std::string line;
                while (std::getline(std::cin, line)) {
                    if (line.empty()) break;
                    commitMsg += line + "\n";
                }
                enableRaw();
                if (!commitMsg.empty()) {
                    std::string tmp = "/tmp/lazygitcpp_commit_msg";
                    FILE* f = fopen(tmp.c_str(), "w");
                    if (f) { fputs(commitMsg.c_str(), f); fclose(f); }
                    std::string out = git("commit -F '" + tmp + "'");
                    if (!out.empty() && out.find("nothing to commit") != std::string::npos)
                        lastError = "Nothing to commit";
                    else
                        lastError = "Committed!";
                }
            }
        }
        else if (c == 'p') { /* push */
            git("push");
            lastError = "Pushed";
        }
        else if (c == 'P') { /* pull */
            lastError = git("pull");
        }
        else if (c == 'h') { /* help overlay */
            /* Simple inline help */
        }
    }

    refresh.join();
    printf("\033[2J\033[H");
    return 0;
}
