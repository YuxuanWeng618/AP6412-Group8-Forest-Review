#include "ForestHUD.h"
#include "ForestGameMode.h"
#include "ForestController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "CanvasItem.h"

namespace
{
FLinearColor Ink(0.035f,0.075f,0.065f,1);
FLinearColor Paper(0.91f,0.89f,0.77f,1);
FLinearColor Muted(0.42f,0.55f,0.43f,1);
FLinearColor Gold(0.96f,0.64f,0.23f,1);
FLinearColor Mint(0.35f,0.78f,0.57f,1);
FLinearColor Rose(0.89f,0.40f,0.34f,1);
FLinearColor Wood(0.39f,0.22f,0.105f,1);
}
void AForestHUD::Box(float x,float y,float w,float h,FLinearColor c)
{ DrawRect(c,Offset.X+x*Zoom,Offset.Y+y*Zoom,w*Zoom,h*Zoom); }
void AForestHUD::Disc(float x,float y,float rx,float ry,FLinearColor c)
{ Canvas->K2_DrawPolygon(nullptr,Offset+FVector2D(x,y)*Zoom,FVector2D(rx,ry)*Zoom,32,c); }
void AForestHUD::Line(float x,float y,float x2,float y2,FLinearColor c,float width)
{ DrawLine(Offset.X+x*Zoom,Offset.Y+y*Zoom,Offset.X+x2*Zoom,Offset.Y+y2*Zoom,c,width*Zoom); }
void AForestHUD::Text(const FString& value,float x,float y,float size,FLinearColor c)
{
    UFont* font = GEngine->GetMediumFont();
    FCanvasTextItem item(Offset+FVector2D(x,y)*Zoom,FText::FromString(value),font,c);
    const float scale = size / FMath::Max(1.f,static_cast<float>(font->GetMaxCharHeight()));
    item.Scale = FVector2D(scale*Zoom,scale*Zoom);
    Canvas->DrawItem(item);
}
void AForestHUD::Leaf(float x,float y,float size,FLinearColor color)
{
    Disc(x,y,size,size*0.48f,color);
    Line(x-size*0.6f,y+1,x+size*0.6f,y-1,Ink,1);
}
void AForestHUD::Elf(FVector2D p,FLinearColor color,int player)
{
    const float x=p.X,y=p.Y;
    Disc(x,y+14,23,9,FLinearColor(0.03f,0.06f,0.04f,0.35f));
    Box(x-13,y+4,9,15,Wood); Box(x+4,y+4,9,15,Wood);
    Disc(x,y,19,23,color);
    Disc(x,y-15,16,15,Paper);
    Disc(x-17,y-16,7,5,Paper); Disc(x+17,y-16,7,5,Paper);
    Disc(x-5,y-15,2,3,Ink); Disc(x+5,y-15,2,3,Ink);
    Disc(x,y-29,20,10,color);
    Leaf(x+8,y-40,13,Mint);
    Box(x-12,y+2,24,4,Ink);
    Disc(x,y+4,3,3,Gold);
    if(player>=0)
    {
        Disc(x+24,y-30,11,11,Ink);
        Text(FString::FromInt(player+1),x+20,y-39,17,Paper);
    }
}
void AForestHUD::DrawItem(const FForestItem& item)
{
    if(item.Consumed) return;
    const float x=item.Position.X,y=item.Position.Y;
    Disc(x,y+11,25,7,FLinearColor(0.025f,0.04f,0.03f,0.35f));
    switch(item.Kind)
    {
    case Forest::ItemKind::Log:
        Box(x-27,y-10,54,22,Wood); Disc(x-27,y+1,9,12,Gold); Disc(x+27,y+1,9,12,Paper);
        Disc(x+27,y+1,5,7,Wood); Line(x-15,y-5,x+15,y-5,Gold,2); break;
    case Forest::ItemKind::Plank:
        Box(x-24,y-8,48,18,Gold); Box(x-24,y+7,48,4,Wood);
        Line(x-18,y-2,x+18,y-2,Wood,1); Line(x-10,y+3,x+10,y+3,Wood,1); break;
    case Forest::ItemKind::Package:
        Box(x-19,y-18,38,34,Gold); Box(x-3,y-18,6,34,Rose); Box(x-19,y-4,38,6,Rose);
        Leaf(x-8,y-20,9,Rose); Leaf(x+8,y-20,9,Rose); break;
    case Forest::ItemKind::Stone:
        Disc(x,y,20,13,FLinearColor(0.38f,0.44f,0.41f,1)); Line(x-7,y-6,x+5,y-7,Muted,3); break;
    }
}
void AForestHUD::DrawWorld(const FPlanarWorld& b)
{
    using namespace Forest;
    // Entire world is canvas geometry. No 3D mesh/physics or camera projection.
    Box(40,158,1360,590,FLinearColor(0.10f,0.19f,0.12f,1));
    Box(53,171,1334,564,FLinearColor(0.19f,0.29f,0.17f,1));
    for(int i=0;i<170;++i)
    {
        float x=70+(i*137%1290),y=185+(i*83%535);
        Line(x,y,x-3,y-5,FLinearColor(0.26f,0.36f,0.20f,1),1);
        Line(x,y,x+4,y-4,FLinearColor(0.26f,0.36f,0.20f,1),1);
    }
    const FLinearColor path(0.34f,0.37f,0.22f,1);
    Box(180,375,1050,45,path); Box(1190,310,40,340,path); Box(530,610,700,40,path); Box(270,397,45,245,path);
    Box(535,485,35,165,path);
    for(int i=0;i<30;++i) Disc(340+i*28,396+(i%3-1)*9,5,3,FLinearColor(0.43f,0.44f,0.28f,1));
    // Pond is also a collision obstacle.
    Disc(728,285,97,65,FLinearColor(0.12f,0.22f,0.16f,1));
    Box(650,225,155,105,FLinearColor(0.19f,0.40f,0.39f,1));
    for(int i=0;i<5;++i) Line(668+i*11,244+i*14,710+i*13,244+i*14,FLinearColor(0.36f,0.57f,0.49f,1),2);
    Leaf(777,312,12,Mint); Disc(777,308,5,5,Paper);
    // Grandfather's cottage.
    Box(105,290,120,65,Wood); Box(117,309,24,28,Gold); Box(171,310,30,45,Ink);
    for(int i=0;i<4;++i) Box(95+i*10,270-i*10,140-i*20,12,Rose);
    Box(100,354,130,7,Ink);
    Elf(FVector2D(180,430),Muted,-1);
    Text(TEXT("GRANDFATHER"),106,476,15,Paper);
    if(b.Session.GetStage()==Stage::Complete)
    {
        Box(120,524,50,12,Gold); Box(120,493,8,60,Wood); Box(162,493,8,60,Wood);
        Line(112,558,178,558,Gold,5); Box(125,497,40,20,Gold);
    }
    // Harvest mushroom.
    Disc(1120,273,75,18,Ink); Box(1099,218,42,62,Paper);
    Disc(1120,213,83,40,Rose); Disc(1120,197,65,28,Rose);
    Disc(1084,206,13,9,Paper); Disc(1134,191,14,10,Paper); Disc(1162,214,10,7,Paper);
    Text(TEXT("01  /  MUSHROOM GROVE"),997,174,15,Paper);
    // Saw table and toothed blade.
    Box(975,445,170,70,Wood); Box(970,445,180,13,Gold);
    for(int i=0;i<8;++i) Line(987+i*20,463,987+i*20,507,FLinearColor(0.50f,0.30f,0.13f,1),2);
    Disc(1104,465,28,28,Paper); Disc(1104,465,8,8,Ink);
    for(int i=0;i<12;++i)
    { float a=i*PI/6; Line(1104+FMath::Cos(a)*25,465+FMath::Sin(a)*25,1104+FMath::Cos(a)*33,465+FMath::Sin(a)*33,Paper,4); }
    Disc(1008,473,24,24,Ink); Disc(1008,473,17,17,Gold); Disc(1008,473,10,10,Wood);
    Text(TEXT("02  /  SAWMILL"),980,414,15,Paper);
    // Assembly flower, ropes and recipe counter.
    for(int i=0;i<6;++i)
    { float a=i*PI/3; Disc(550+FMath::Cos(a)*30,440+FMath::Sin(a)*22,24,17,Muted); }
    Disc(550,440,26,23,Gold); Disc(550,440,13,12,Wood);
    Line(520,462,440,510,Gold,4); Line(580,462,660,510,Gold,4);
    Text(TEXT("03  /  ASSEMBLY FLOWER"),440,376,15,Paper);
    for(int i=0;i<3;++i)
    {
        Box(509+i*30,481,23,12,i<b.Session.GetRecipe().Count()?Gold:Ink);
        Line(512+i*30,486,527+i*30,486,Wood,1);
    }
    // Decorative perimeter shrubs, outside walkable boundary.
    for(int i=0;i<16;++i)
    {
        float x=70+i*85;
        Disc(x,727,29,17,FLinearColor(0.10f,0.22f,0.14f,1));
        Leaf(x-10,721,14,Muted);
    }
    const Stage stage=b.Session.GetStage();
    FVector2D target=b.Elder;
    if(stage==Stage::Discover || stage==Stage::Harvest) target=FVector2D(1120,300);
    if(stage==Stage::ReturnLog || stage==Stage::LoadSaw || stage==Stage::Saw) target=FVector2D(1060,535);
    if(stage==Stage::Supply || stage==Stage::Ropes) target=FVector2D(550,530);
    if(stage==Stage::Deliver) target=FVector2D(180,430);
    if(stage!=Stage::Complete)
    {
        Disc(target.X,target.Y+43,6,6,Gold);
        Line(target.X-12,target.Y+32,target.X,target.Y+42,Gold,3);
        Line(target.X+12,target.Y+32,target.X,target.Y+42,Gold,3);
    }
    if(stage==Stage::Harvest || stage==Stage::Saw || stage==Stage::Ropes)
    {
        for(int s=0;s<2;++s)
        {
            FVector2D p=b.StationPosition(s);
            const int owner=b.Session.GetStations().Owner(s);
            const FLinearColor color=owner<0?Gold:owner==0?Mint:Rose;
            Disc(p.X,p.Y,30,25,color); Disc(p.X,p.Y,25,20,path);
            Text(stage==Stage::Saw?(s==0?TEXT("WHEEL"):TEXT("BLADE")):stage==Stage::Ropes?TEXT("ROPE"):TEXT("HARVEST"),p.X-32,p.Y+30,13,Paper);
            if(stage!=Stage::Ropes)
            {
                Box(p.X-35,p.Y+51,70,5,Ink);
                Box(p.X-35,p.Y+51,70*b.Session.GetQte().Progress(s)/8.f,5,color);
            }
        }
    }
    for(const auto& item:b.Items) if(item.HeldBy<0) DrawItem(item);
    for(int p=0;p<2;++p) Elf(b.Players[p].Position,p==0?Mint:Rose,p);
    for(const auto& item:b.Items) if(item.HeldBy>=0) DrawItem(item);
    Text(TEXT("Stone: wrong material"),802,688,13,Muted);
}
void AForestHUD::NotifyHitBoxClick(FName BoxName)
{
    Super::NotifyHitBoxClick(BoxName);
    if (auto* controller=Cast<AForestController>(GetOwningPlayerController())) controller->HandleMenuAction(BoxName);
}
void AForestHUD::Button(FName action,const FString& label,float x,float y,float width,bool primary)
{
    Box(x,y,width,52,primary?Gold:Muted);
    Text(label,x+18,y+15,20,Ink);
    AddHitBox(Offset+FVector2D(x,y)*Zoom,FVector2D(width,52)*Zoom,action,true);
}
void AForestHUD::DrawOverlay(const FPlanarWorld& b)
{
    using namespace Forest;
    if(b.Started && !b.Help && !b.Paused && b.Session.GetStage()!=Stage::Complete) return;
    Box(40,158,1360,590,FLinearColor(0.02f,0.05f,0.04f,0.82f));
    Box(300,220,840,455,Ink); Box(300,220,840,4,Gold);
    if(!b.Started)
    {
        Text(TEXT("A LITTLE WORKSHOP. A SHARED ADVENTURE."),370,260,16,Mint);
        Text(TEXT("Made together."),370,301,58,Paper);
        Text(TEXT("Two forest helpers. One rocking chair. Your teamwork."),370,383,22,Paper);
        Text(TEXT("Accept an order, harvest a log, saw three planks,"),370,428,20,Muted);
        Text(TEXT("pull the ropes together, and bring Grandfather his chair."),370,457,20,Muted);
        Button(TEXT("Start"),TEXT("ENTER / Play"),370,523,220,true);
        Button(TEXT("Demo"),TEXT("V / Watch demo"),610,523,240);
        Button(TEXT("Quit"),TEXT("Q / Quit game"),870,523,220);
        Text(TEXT("One keyboard  /  Two players  /  No time limit"),370,615,17,Muted);
    }
    else if(b.Help)
    {
        Text(TEXT("A guide to working together"),355,253,35,Paper);
        Text(TEXT("PLAYER 1"),355,320,18,Mint); Text(TEXT("PLAYER 2"),750,320,18,Rose);
        Text(TEXT("W A S D        Move / turn wheel"),355,355,19,Paper);
        Text(TEXT("Arrow keys    Move / turn wheel"),750,355,19,Paper);
        Text(TEXT("E                 Pick up / put down"),355,392,19,Paper);
        Text(TEXT("J                  Pick up / put down"),750,392,19,Paper);
        Text(TEXT("F                 Enter / leave station"),355,429,19,Paper);
        Text(TEXT("K                  Enter / leave station"),750,429,19,Paper);
        Text(TEXT("Space          Blade / hold rope"),355,466,19,Paper);
        Text(TEXT("Right Shift   Blade / hold rope"),750,466,19,Paper);
        Text(TEXT("Harvest: alternate left and right. Both station circles must be occupied."),355,523,18,Gold);
        Text(TEXT("Drop three planks near the flower. Join both ropes; hold both action keys."),355,555,18,Gold);
        Text(TEXT("H  close guide    P / Esc  pause    F5  restart"),355,618,17,Muted);
        Button(TEXT("Quit"),TEXT("Q / Quit game"),870,600,220);
    }
    else if(b.Session.GetStage()==Stage::Complete)
    {
        Text(TEXT("ORDER COMPLETE"),370,270,17,Mint);
        Text(TEXT("A chair. A little joy."),370,315,51,Paper);
        Text(TEXT("Grandfather's rocking chair is ready."),370,397,24,Paper);
        Text(TEXT("Harvested, crafted and delivered by the two of you."),370,443,21,Muted);
        Text(FString::Printf(TEXT("3 planks   /   2 helpers   /   %d:%02d together"),static_cast<int>(b.PlaySeconds)/60,static_cast<int>(b.PlaySeconds)%60),370,495,21,Gold);
        Button(TEXT("Start"),TEXT("ENTER / Play again"),370,555,310,true);
        Button(TEXT("Quit"),TEXT("Q / Quit game"),710,555,260);
    }
    else
    {
        Text(TEXT("Take a breath."),370,300,56,Paper);
        Text(TEXT("Your order is waiting right here."),370,395,24,Muted);
        Button(TEXT("Resume"),TEXT("P / ESC / Resume"),370,510,290,true);
        Button(TEXT("Quit"),TEXT("Q / Quit game"),690,510,280);
        Text(TEXT("H  controls     F5  restart"),370,596,20,Muted);
    }
}
void AForestHUD::DrawHUD()
{
    Super::DrawHUD();
    if(!Canvas) return;
    auto* game=GetWorld()->GetAuthGameMode<AForestGameMode>();
    if(!game) return;
    const auto& b=game->Board;
    DrawRect(Ink,0,0,Canvas->ClipX,Canvas->ClipY);
    Zoom=FMath::Min(Canvas->ClipX/1440.f,Canvas->ClipY/900.f);
    Offset=FVector2D((Canvas->ClipX-1440*Zoom)/2,(Canvas->ClipY-900*Zoom)/2);
    Leaf(62,52,20,Mint);
    Text(TEXT("forest"),95,18,57,Paper);
    Text(TEXT("TOGETHER / 2D"),285,45,17,Mint);
    Text(TEXT("A COOPERATIVE WOODLAND WORKSHOP"),900,43,17,Muted);
    Line(40,96,1400,96,Muted,1);
    const int stage=static_cast<int>(b.Session.GetStage());
    Text(stage==9?TEXT("ORDER FINISHED"):FString::Printf(TEXT("ORDER 01    /    STEP %02d OF 09"),stage+1),40,118,17,Gold);
    Text(TEXT("H  guide     P  pause     F5  restart"),1080,118,16,Muted);
    DrawWorld(b);
    Box(40,762,1360,117,FLinearColor(0.055f,0.105f,0.08f,1));
    Box(40,762,5,117,Gold);
    Text(b.Objective(),65,778,25,Paper);
    Text(TEXT("01"),65,830,19,Mint); Text(b.Hint(0),104,830,18,Paper);
    Text(TEXT("02"),755,830,19,Rose); Text(b.Hint(1),794,830,18,Paper);
    if(b.NoticeSeconds>0 && b.Started && !b.Paused && !b.Help)
    {
        Box(200,682,1040,42,Ink); Text(b.Notice,220,694,19,Gold);
    }
    DrawOverlay(b);
    if (const auto* controller=Cast<AForestController>(GetOwningPlayerController()); controller && controller->IsDemonstrating())
    {
        Box(450,106,510,38,Ink);
        Text(TEXT("WATCHING DEMO   /   ESC to take control"),470,117,17,Gold);
    }
}
