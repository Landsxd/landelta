// Run in Windows CI before normal startup: no user settings/files are touched.
bool studioSelfTest(){
 cfg=Config{};cfg.gameMode=1;studioPage=2;studioPreview=true;beginLayout();
 const float originalScale=cfg.scale,originalDelta=cfg.deltaScale;bool visible=cfg.visible[3];
 studioAction(21);studioAction(28);studioAction(13);
 studioAction(202);
 if(!layoutDirty||cfg.scale!=originalScale||cfg.deltaScale!=originalDelta||cfg.visible[3]!=visible)return false;
 if(draftScale<=originalScale||draftDelta<=originalDelta||draftVisible[3]==visible)return false;
 studioAction(230);
 if(layoutDirty||draftScale!=originalScale||draftDelta!=originalDelta||draftVisible[3]!=visible)return false;
 Bitmap bmp(1200,780,PixelFormat32bppARGB);Graphics g(&bmp);renderLayout(g);
 studioAction(21);studioAction(28);studioAction(13);draftX[4]=500;draftY[4]=500;studioAction(232);
 if(layoutDirty||cfg.scale<=originalScale||cfg.deltaScale<=originalDelta||cfg.visible[3]==visible||cfg.x[4]!=500||cfg.y[4]!=500)return false;
 cfg.saveProfile();float accScale=cfg.scale;cfg.gameMode=2;cfg.scale=.8f;cfg.saveProfile();cfg.gameMode=1;cfg.restoreProfile();
 if(cfg.scale!=accScale||cfg.visible[3]==visible)return false;
 // Moving a panel to monitor 2 preserves the negative desktop origin on save.
 selectedMonitor=0;beginLayout();studioAction(251);studioAction(14);renderLayout(g);studioAction(232);
 if(cfg.x[4]>=0||cfg.x[4]<-2560||cfg.y[4]<-200)return false;
 beginLayout();return draftMonitor[4]==1;
}
