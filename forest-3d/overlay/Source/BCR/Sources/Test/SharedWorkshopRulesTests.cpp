#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "BCR/Headers/System/BCRWorkshopRulesBridge.h"

// The workshop checks that used to run against the C# rules, now run against the
// UE adapter that delegates to the shared Forest rules (forest-2d Domain).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBCRSharedWorkshopRulesTest, "BCR.SharedRules.Workshop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBCRSharedWorkshopRulesTest::RunTest(const FString& Parameters)
{
    enum { Inactive, Waiting, Running, Paused, Completed, Failed };
    enum { Rejected, Accepted, AlreadyPresent };

    UBCRWorkshopRulesBridge* Stations = UBCRWorkshopRulesBridge::Create(GetTransientPackage());
    TestTrue(TEXT("unloaded station rejects entry"),
        Stations->InteractWithStation(0, false, 1, 5) == INDEX_NONE && !Stations->AreStationsFilled());
    TestTrue(TEXT("nearest occupied station preserves its owner"),
        Stations->InteractWithStation(0, true, 1, 5) == 0 && Stations->InteractWithStation(1, true, 1, 5) == INDEX_NONE);
    TestTrue(TEXT("two players occupy opposite stations and leave without supplies"),
        Stations->InteractWithStation(1, true, 5, 1) == 1 && Stations->AreStationsFilled() &&
        Stations->InteractWithStation(0, false, 1, 5) == 2);
    TestTrue(TEXT("only the two local players can hold a station"), Stations->InteractWithStation(7, true, 1, 5) == INDEX_NONE);

    UBCRWorkshopRulesBridge* Machine = UBCRWorkshopRulesBridge::Create(GetTransientPackage());
    Machine->AddMachineRequirement(TEXT("LongLog"));
    Machine->AddMachineRequirement(TEXT("Peg"));
    TestTrue(TEXT("machine preserves recipe indices and name matching"),
        Machine->FindMachineRequirement(TEXT("longlog")) == 0 && Machine->FindMachineRequirement(TEXT("Peg")) == 1 &&
        Machine->FindMachineRequirement(TEXT("Plank")) == INDEX_NONE);
    TestTrue(TEXT("machine rejects wrong material or deposits outside the input box"),
        Machine->DepositMachineItem(10, TEXT("Plank"), true) == Rejected &&
        Machine->DepositMachineItem(10, TEXT("LongLog"), false) == Rejected && !Machine->HasMachineItems());
    Machine->StageMachineReplacement(0);
    TestTrue(TEXT("missing replacement leaves the original requirement"),
        !Machine->RegisterMachineReplacement(-1) && Machine->FindMachineRequirement(TEXT("LongLog")) == 0);
    Machine->StageMachineReplacement(0);
    TestTrue(TEXT("replacement fulfils exactly its selected requirement"),
        Machine->RegisterMachineReplacement(20) && Machine->FindMachineRequirement(TEXT("LongLog")) == INDEX_NONE &&
        Machine->FindMachineRequirement(TEXT("Peg")) == 0 && !Machine->IsMachineSupplied());
    TestTrue(TEXT("a duplicate actor cannot consume a second requirement"),
        Machine->DepositMachineItem(20, TEXT("Peg"), true) == AlreadyPresent &&
        Machine->DepositMachineItem(21, TEXT("Peg"), true) == Accepted && Machine->IsMachineSupplied());
    Machine->ForgetMachineItem(20);
    Machine->ForgetMachineItem(20);
    TestTrue(TEXT("losing a replacement restores its requirement once"),
        !Machine->IsMachineSupplied() && Machine->FindMachineRequirement(TEXT("LongLog")) == 0 && Machine->HasMachineItems() &&
        Machine->DepositMachineItem(22, TEXT("LongLog"), true) == Accepted && Machine->IsMachineSupplied());

    UBCRWorkshopRulesBridge* Run = UBCRWorkshopRulesBridge::Create(GetTransientPackage());
    Run->AddMachineRequirement(TEXT("LongLog"));
    Run->DepositMachineItem(1, TEXT("LongLog"), true);
    TestFalse(TEXT("machine needs two occupied stations"), Run->CanStartMachineRound(Inactive));
    Run->InteractWithStation(0, true, 1, 5);
    Run->InteractWithStation(1, true, 5, 1);
    TestTrue(TEXT("machine starts with material and two stations, never over an active QTE"),
        Run->CanStartMachineRound(Inactive) && !Run->CanStartMachineRound(Waiting) &&
        !Run->CanStartMachineRound(Running) && !Run->CanStartMachineRound(Paused));
    TestTrue(TEXT("unowned rounds cannot finish"),
        !Run->ConfirmMachineRound(Inactive) && !Run->ShouldFinishSawRound(true) && !Run->ShouldFinishTreeRound(0));
    TestTrue(TEXT("owned machine waits for the authored finish signals"),
        Run->ConfirmMachineRound(Waiting) && Run->OwnsMachineRound() && !Run->ShouldFinishSawRound(false) &&
        Run->ShouldFinishSawRound(true) && !Run->ShouldFinishTreeRound(1) && Run->ShouldFinishTreeRound(0));
    Run->ReleaseMachineRound();
    TestTrue(TEXT("finishing releases ownership and allows another round"),
        !Run->OwnsMachineRound() && Run->CanStartMachineRound(Completed));
    TestTrue(TEXT("replacement unlocks empty hands or the replaced log only"),
        Run->ShouldUnlockReplacementCarry(false, false) && Run->ShouldUnlockReplacementCarry(true, true) &&
        !Run->ShouldUnlockReplacementCarry(true, false));

    UBCRWorkshopRulesBridge* Recipe = UBCRWorkshopRulesBridge::Create(GetTransientPackage());
    Recipe->ResetRecipe();
    Recipe->AddIngredient(21, 3);
    TestTrue(TEXT("recipe rejects wrong type and duplicate item"),
        !Recipe->ReserveIngredient(1, 99) && Recipe->ReserveIngredient(1, 21) && !Recipe->ReserveIngredient(1, 21));
    TestTrue(TEXT("simultaneous deposits reserve three places without consuming them"),
        Recipe->ReserveIngredient(2, 21) && Recipe->ReserveIngredient(3, 21) && !Recipe->ReserveIngredient(4, 21) &&
        Recipe->RemainingTotal() == 3);
    TestFalse(TEXT("ropes are ignored before the recipe is complete"), Recipe->ReserveCraft(0, 1, true, true));
    TestTrue(TEXT("failed consumption frees its reservation; a third distinct plank completes the recipe"),
        Recipe->FinishIngredient(1, true) && !Recipe->FinishIngredient(1, true) && !Recipe->FinishIngredient(2, false) &&
        Recipe->RemainingTotal() == 2 && Recipe->ReserveIngredient(4, 21) && Recipe->FinishIngredient(3, true) &&
        Recipe->FinishIngredient(4, true) && Recipe->RemainingTotal() == 0);
    TestTrue(TEXT("dropped parts queue only when released in the zone"),
        !Recipe->ShouldQueueIngredient(true, true) && !Recipe->ShouldQueueIngredient(false, false) &&
        Recipe->ShouldQueueIngredient(false, true) && Recipe->CanConsumeIngredient(false, true) &&
        !Recipe->CanConsumeIngredient(true, true));

    Recipe->ObserveRopes(1, 1, true, true);
    TestFalse(TEXT("one player cannot operate both ropes"), Recipe->ReserveCraft(1, 1, false, false));
    Recipe->ObserveRopes(1, 2, true, false);
    TestFalse(TEXT("one pulled rope cannot activate the assembler"), Recipe->ReserveCraft(1, 2, true, false));
    Recipe->ObserveRopes(1, 2, true, true);
    TestTrue(TEXT("a paired pull stays valid across the presentation flag reset, and crafts once"),
        Recipe->ReserveCraft(1, 2, false, false) && !Recipe->ReserveCraft(1, 2, true, true));
    TestTrue(TEXT("a failed spawn can retry; a successful output is committed once"),
        !Recipe->FinishOutput(false) && Recipe->ReserveCraft(1, 2, false, false) && Recipe->FinishOutput(true) &&
        !Recipe->ReserveCraft(1, 2, true, true));

    UBCRWorkshopRulesBridge* Delivery = UBCRWorkshopRulesBridge::Create(GetTransientPackage());
    TestTrue(TEXT("delivery accepts only the required item, once"),
        !Delivery->ReserveDelivery(5, 6) && !Delivery->ReserveDelivery(5, -1) && Delivery->ReserveDelivery(6, 6) &&
        !Delivery->ReserveDelivery(6, 6) && Delivery->FinishOutput(true) && !Delivery->ReserveDelivery(6, 6));
    return true;
}
#endif
