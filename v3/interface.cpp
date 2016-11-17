#include <vector>
#include <termio.h>
#include "interface.h"
#include "editor.h"

using namespace std;


namespace Interface{

	class TermioRAII{
	public:
		TermioRAII(){
			initscreen();
			initkeyboard(true);
			installCLhandler(true);
		}

		~TermioRAII(){
			endkeyboard();
			endscreen();
		}
	};

	static void printStatus(const char *str){
		Size termsize=gettermsize();
		pushcursor();
		moveto(0,termsize.h-1);
		Style style={.fg=9,.bg=9,.bold=true,.ul=false};
		setstyle(&style);
		tprintf("%s",str);
		popcursor();
	}

	static bool confirmStatus(const char *str,bool defChoice){
		Size termsize=gettermsize();
		pushcursor();
		moveto(0,termsize.h-1);
		Style style={.fg=9,.bg=9,.bold=true,.ul=false};
		setstyle(&style);
		tprintf("%s [%s] ",str,defChoice?"Y/n":"y/N");
		redraw();
		int key=tgetkey();
		bool choice;
		if(defChoice==true){
			choice=key!='n'&&key!='N';
		} else {
			choice=key=='Y'||key=='y';
		}
		fillrect(0,termsize.h-1,termsize.w,1,' ');
		redraw();
		popcursor();
		return choice;
	}

	void show(){
		TermioRAII termioRAII;
		i64 tabscroll=0;
		Editor editor([](i64 &x,i64 &y,i64 &w,i64 &h){
			Size termsize=gettermsize();
			x=0; y=1;
			w=termsize.w; h=termsize.h-2;
		});

		while(true){
			Size termsize=gettermsize();

			i64 nviews=editor.numViews();
			i64 tabpos[nviews],tabwid[nviews];
			tabpos[0]=0;
			tabwid[0]=min((int)editor.view(0).getName().size()+2,termsize.w-4);
			for(i64 i=1;i<nviews;i++){
				tabpos[i]=tabpos[i-1]+tabwid[i-1];
				tabwid[i]=min((int)editor.view(i).getName().size()+2,termsize.w-4);
			}

			i64 activeidx=editor.activeIndex();

			if(tabpos[activeidx]-tabscroll<0){
				tabscroll=max(tabpos[activeidx]-1,0LL);
			} else if(tabpos[activeidx]+tabwid[activeidx]-tabscroll>=termsize.w){
				tabscroll=max(tabpos[activeidx]+tabwid[activeidx]-(termsize.w-2),0LL);
			}

			moveto(0,0);
			Style blankStyle={.fg=9,.bg=9,.bold=false,.ul=false};
			setstyle(&blankStyle);
			fillrect(0,0,termsize.w,1,' ');

			for(i64 i=0;i<nviews;i++){
				Style style={.fg=7,.bg=i==activeidx?4:9,.bold=false,.ul=false};
				setstyle(&style);
				const string &name=editor.view(i).getName();
				if(tabpos[i]<tabscroll&&tabpos[i]+tabwid[i]>=tabscroll){
					if(tabpos[i]+tabwid[i]==tabscroll){
						tputc(' ');
					} else {
						for(i64 j=tabscroll-tabpos[i]-1;j<tabwid[i];j++){
							tputc(name[j]);
						}
						tputc(' ');
					}
				} else if(tabpos[i]>=tabscroll&&tabpos[i]+tabwid[i]<tabscroll+termsize.w){
					tputc(' ');
					for(i64 j=0;j<tabwid[i]-2;j++){
						tputc(name[j]);
					}
					tputc(' ');
				} else if(tabpos[i]<tabscroll+termsize.w&&tabpos[i]+tabwid[i]>=tabscroll+termsize.w){
					tputc(' ');
					for(i64 j=0;j<tabscroll+termsize.w-tabpos[i]-1;j++){
						tputc(name[j]);
					}
					break;
				}
			}

			editor.drawActive();

			redraw();

			int key=tgetkey();
			cerr<<"tgetkey -> "<<key<<endl;
			switch(key){
				case KEY_CTRL+'Q':
					return;

				case KEY_CTRL+'N':
					editor.newView();
					break;

				case KEY_CTRL+'W':
					if(confirmStatus("Close buffer?",true))editor.closeView();
					break;

				case KEY_ALT+KEY_TAB:
					editor.setActiveIndex((editor.activeIndex()+1)%editor.numViews());
					break;

				case KEY_ALT+KEY_SHIFTTAB:
					editor.setActiveIndex((editor.activeIndex()+editor.numViews()-1)%editor.numViews());
					break;

				default:
					if(!editor.handleKey(key)){
						printStatus("Unknown key");
						bel();
					}
					break;
			}
		}
	}

}