#include "Misc/AutomationTest.h"
#include "Domain/WorkshopSession.h"

#if WITH_DEV_AUTOMATION_TESTS
// Rules added to the shared core for the 3D workshop. The 2D session uses them too
// (PlankRecipe is an ItemRecipe, Pull goes through RopePair).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestSharedRulesTest, "Forest2D.Rules.SharedRecipeRopesOutput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FForestSharedRulesTest::RunTest(const FString& Parameters)
{
    using namespace Forest;
    ItemRecipe Recipe;
    Recipe.Require(21, 3);
    Recipe.Require(21, 5);
    TestTrue(TEXT("a type is required once"), Recipe.RemainingTotal() == 3 && !Recipe.Ready());
    TestTrue(TEXT("wrong type and duplicate reservations rejected"),
        !Recipe.Reserve(1, 99) && Recipe.Reserve(1, 21) && !Recipe.Reserve(1, 21) && !Recipe.Reserve(-1, 21));
    TestTrue(TEXT("reservations never exceed the requirement"),
        Recipe.Reserve(2, 21) && Recipe.Reserve(3, 21) && !Recipe.Reserve(4, 21) && Recipe.RemainingTotal() == 3);
    TestTrue(TEXT("a failed consumption frees its place"),
        Recipe.Finish(1, true) && !Recipe.Finish(1, true) && !Recipe.Finish(2, false) && Recipe.Reserve(4, 21));
    TestTrue(TEXT("three distinct items complete the recipe"),
        Recipe.Finish(3, true) && Recipe.Finish(4, true) && Recipe.Ready() && !Recipe.Reserve(5, 21));

    PlankRecipe Planks;
    TestTrue(TEXT("the 2D recipe counts three distinct planks"),
        !Planks.Consume(1, ItemKind::Log) && Planks.Consume(1, ItemKind::Plank) && !Planks.Consume(1, ItemKind::Plank) &&
        Planks.Consume(2, ItemKind::Plank) && Planks.Count() == 2 && Planks.Consume(3, ItemKind::Plank) &&
        Planks.Ready() && !Planks.Consume(4, ItemKind::Plank) && Planks.Count() == PlankRecipe::Required);

    RopePair Ropes;
    Ropes.Observe(1, 1, true, true);
    TestFalse(TEXT("one player cannot pull both ropes"), Ropes.CanActivate(1, 1));
    Ropes.Observe(1, 2, true, false);
    TestFalse(TEXT("one rope is not enough"), Ropes.CanActivate(1, 2));
    Ropes.Observe(1, 2, true, true);
    Ropes.Observe(1, 2, false, false);
    TestTrue(TEXT("a paired pull stays valid for the same holders"), Ropes.CanActivate(1, 2));
    Ropes.Observe(1, 3, true, false);
    TestTrue(TEXT("changing a holder clears the pair"), !Ropes.CanActivate(1, 3) && !Ropes.CanActivate(1, 2));

    OutputGate Output;
    TestTrue(TEXT("output rejects ineligible and re-entrant requests"),
        !Output.Reserve(false) && Output.Reserve(true) && !Output.Reserve(true));
    TestTrue(TEXT("a failed creation can retry; success is final"),
        !Output.Finish(false) && Output.Reserve(true) && Output.Finish(true) && Output.Completed() && !Output.Reserve(true));
    return true;
}
#endif
