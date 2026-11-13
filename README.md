# Forest Together · AP6412 Group 8

Forest Together is a local two-player workshop game built in Unreal Engine. Players share a keyboard to accept an order, gather and carry a log, cut it into three pieces, assemble a rocking chair, and deliver it.

| Version | Start here |
| --- | --- |
| 2D | [Source and keyboard controls](forest-2d/README.md). Open `forest-2d/Forest2D.uproject` in Unreal Engine 5.8.1 on Windows. Load `/Game/Maps/Forest`, click **Play**, and press **Enter**. The menu also has **Watch demo**. |
| 3D | [Reconstruction, build and play](forest-3d/README.md). Download the pinned Project Becorn dependency using `Prepare-3D.ps1`, then build the project with Unreal Engine 5.8.1 and the .NET 10 SDK. |

[Frozen 2D baseline](versions/2d-baseline/) · [2D v1.1](versions/2d-v1.1/) · [Route B comparison and class maps](docs/ROUTE_B_COMPARISON.md) · [AI dialogue excerpts](docs/AI-EXCERPTS.md)

## Development sequence

The team first designed and implemented an early 3D game flow inspired by the cooperation in *It Takes Two* and *Overcooked*. We then froze a standalone playable 2D baseline and integrated its shared workshop rules into the final 3D project. Both versions compile [WorkshopSession.h](forest-2d/Source/Forest2D/Domain/WorkshopSession.h) for station assignment, recipes, paired rope input and output gating.

This public review snapshot uses a fresh Git history. The folder names identify the course milestones. The signed coursework records and original conversation archive remain in the private submission.

## Credits

The original 3D levels, characters and art are from [Project Becorn by Oakfolks, LeoSery and collaborators](https://github.com/LeoSery/Project-Becorn--UnrealEngine5-2024/tree/d35718cfe66854e0a9b40ea06d44e4ce4fa44b1f). The 3D setup fetches them from the authors' repository. The project uses [UnrealSharp](https://github.com/UnrealSharp/UnrealSharp), whose [MIT license](forest-3d/overlay/Plugins/UnrealSharp/LICENSE) is included. The upstream 3D project also includes the Rokoko retargeting plugin and its license. Unreal Engine is installed separately.
