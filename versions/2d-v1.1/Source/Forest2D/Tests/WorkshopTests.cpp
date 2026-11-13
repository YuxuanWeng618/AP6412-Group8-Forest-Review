#include "Misc/AutomationTest.h"
#include "PlanarWorld.h"
#include "Tests/OrderReplay.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestDomainTest,"Forest2D.Rules.OrderAndInvalidActions",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FForestDomainTest::RunTest(const FString& Parameters)
{
    using namespace Forest;
    WorkshopSession s;
    TestFalse(TEXT("cannot deliver before order"),s.Deliver(ItemKind::Package));
    TestFalse(TEXT("cannot join inactive station"),s.Join(0,0));
    TestTrue(TEXT("accept order once"),s.AcceptOrder());
    TestFalse(TEXT("duplicate order rejected"),s.AcceptOrder());
    TestTrue(TEXT("discover grove"),s.Discover());
    TestTrue(TEXT("first player joins"),s.Join(0,0));
    TestFalse(TEXT("same player cannot occupy both stations"),s.Join(1,0));
    TestFalse(TEXT("single player cannot harvest"),s.Work(0,1,false));
    TestTrue(TEXT("second player joins"),s.Join(1,1));
    TestFalse(TEXT("harvesting rejects vertical direction"),s.Work(0,0,false));
    TestTrue(TEXT("first horizontal edge accepted"),s.Work(0,1,false));
    TestFalse(TEXT("repeated same edge rejected"),s.Work(0,1,false));
    for(int i=0;i<8;++i) { s.Work(0,i%2?1:3,false); s.Work(1,i%2?1:3,false); }
    TestTrue(TEXT("harvest produces return stage"),s.GetStage()==Stage::ReturnLog);
    TestTrue(TEXT("return log"),s.ReturnLog());
    TestFalse(TEXT("stone cannot load saw"),s.LoadSaw(ItemKind::Stone));
    TestTrue(TEXT("log loads saw"),s.LoadSaw(ItemKind::Log));
    TestTrue(TEXT("P2 can use wheel"),s.Join(0,1));
    TestTrue(TEXT("P1 can use blade"),s.Join(1,0));
    TestFalse(TEXT("direction alone cannot operate blade"),s.Work(0,1,false));
    for(int i=0;i<8;++i) { s.Work(1,i%4,false); s.Work(0,-1,true); }
    TestTrue(TEXT("saw completes"),s.GetStage()==Stage::Supply);
    TestFalse(TEXT("wrong ingredient rejected"),s.Supply(1,ItemKind::Stone));
    TestFalse(TEXT("invalid item ID rejected"),s.Supply(-1,ItemKind::Plank));
    TestTrue(TEXT("first plank"),s.Supply(1,ItemKind::Plank));
    TestFalse(TEXT("duplicate plank"),s.Supply(1,ItemKind::Plank));
    TestTrue(TEXT("second plank"),s.Supply(2,ItemKind::Plank));
    TestFalse(TEXT("cannot pull with only two planks"),s.Pull(true,true));
    TestTrue(TEXT("third plank"),s.Supply(3,ItemKind::Plank));
    TestFalse(TEXT("overfilling rejected"),s.Supply(4,ItemKind::Plank));
    TestTrue(TEXT("left rope"),s.Join(0,0));
    TestTrue(TEXT("right rope"),s.Join(1,1));
    TestFalse(TEXT("single rope action rejected"),s.Pull(true,false));
    TestTrue(TEXT("leave and replace rope owner"),s.Leave(0));
    TestFalse(TEXT("missing rope owner rejected"),s.Pull(true,true));
    TestTrue(TEXT("rejoin"),s.Join(0,0));
    TestTrue(TEXT("both ropes craft package"),s.Pull(true,true));
    TestFalse(TEXT("duplicate package spawn rejected"),s.Pull(true,true));
    TestFalse(TEXT("wrong delivery type"),s.Deliver(ItemKind::Plank));
    TestTrue(TEXT("valid delivery"),s.Deliver(ItemKind::Package));
    TestFalse(TEXT("duplicate delivery"),s.Deliver(ItemKind::Package));
    TestTrue(TEXT("recognizable end"),s.GetStage()==Stage::Complete);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestPlaneTest,"Forest2D.World.MovementPauseAndItems",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FForestPlaneTest::RunTest(const FString& Parameters)
{
    FPlanarWorld world;
    FForestInput inputs[2];
    const FVector2D initial=world.Players[0].Position;
    inputs[0].Move=FVector2D(1,0);
    world.Update(0.1f,inputs);
    TestTrue(TEXT("menu blocks movement"),world.Players[0].Position.Equals(initial));
    world.Started=true; world.Update(0.1f,inputs);
    TestTrue(TEXT("movement is effective"),world.Players[0].Position.X>initial.X);
    const auto moved=world.Players[0].Position;
    world.Paused=true; world.Update(0.1f,inputs);
    TestTrue(TEXT("pause freezes movement"),world.Players[0].Position.Equals(moved));
    world.Paused=false; world.Help=true; world.Update(0.1f,inputs);
    TestTrue(TEXT("help freezes gameplay"),world.Players[0].Position.Equals(moved));
    world.Help=false;
    TestFalse(TEXT("pond blocks movement"),world.CanStand(FVector2D(700,280)));
    TestFalse(TEXT("world edge blocks movement"),world.CanStand(FVector2D(-10,300)));
    world.Players[0].Position=FVector2D(620,280);
    world.Update(0.1f,inputs);
    TestTrue(TEXT("movement cannot tunnel into pond"),world.Players[0].Position.X<=633);
    inputs[0]={}; inputs[1]={};
    world.Players[0].Position=FVector2D(870,650); world.Players[1].Position=FVector2D(870,650);
    inputs[0].Pick=true; inputs[1].Pick=true; world.Update(0.016f,inputs);
    TestTrue(TEXT("one owner per item"),world.Players[0].HeldItem>=0 && world.Players[1].HeldItem<0);
    inputs[1].Pick=false; world.Update(0.016f,inputs);
    TestTrue(TEXT("drop releases item"),world.Players[0].HeldItem<0);
    world.Reset();
    TestTrue(TEXT("restart clears progress and time"),world.Session.GetStage()==Forest::Stage::Order && world.PlaySeconds==0 && !world.Started);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestOrderReplayTest,"Forest2D.World.CompleteOrderWithoutTeleport",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FForestOrderReplayTest::RunTest(const FString& Parameters)
{
    FPlanarWorld board;
    FOrderReplay replay;
    for(int frame=0;frame<18000 && !replay.Done && replay.Error.IsEmpty();++frame) replay.Tick(board,1.f/60);
    TestTrue(TEXT("input replay finished: ")+replay.Error,replay.Done);
    TestTrue(TEXT("order delivered using normal movement and interactions"),board.Session.GetStage()==Forest::Stage::Complete);
    TestEqual(TEXT("three distinct ingredients consumed"),board.Session.GetRecipe().Count(),3);
    TestTrue(TEXT("players remain inside legal planar world"),board.CanStand(board.Players[0].Position) && board.CanStand(board.Players[1].Position));
    return true;
}
#endif
