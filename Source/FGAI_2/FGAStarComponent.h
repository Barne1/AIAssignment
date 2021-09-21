#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FGAStarComponent.generated.h"

/*
 * AStar Breakdown
 * Each node needs to keep track of what step it is + distance.
 * Combined they give a heuristic value which determines which path to try first. Lower gets tried first.
 * You can also add a G value for the difficulty of moving through a node
 *
 * Add starting node to Queue
 *
 * Remove node from top of Queue, check all its neighbours
 * Calculate Heuristics for the neighbour, and store which node visited it
 * If there is already a previous visitor, check if you should replace it, without using the AStar heuristic value for the calculation
 * Remember to update the value and position in queue if the node was already in the list but not visited
 * Add each neighbour to the queue, with its corresponding heuristics value
 * Nodes with lower ranked Heuristics go to the top of the queue
 * 
 * When you visit a node, add it into a hashset to prevent multiple visits
 *
 * When you reach the end, compile a list of nodes by going back through the "visited by", starting with the end node
 * Don't forget to flip the list (or simply compile it the other way around)
*/

class AFGGridActor;

USTRUCT()
struct FAStarNodeData
{
	GENERATED_BODY()
public:
	FAStarNodeData(){};
	FAStarNodeData(FAStarNodeData* CurrentNode, const FIntPoint Pos){SetupNodeData(CurrentNode, Pos);}
	
	FIntPoint XYPos = FIntPoint(-1,-1);
	FAStarNodeData* PreviousNode = nullptr;
	
	float GetF(FIntPoint EndPos)const {return G + GetH(EndPos);} //Value used for queue
	
	int G = INT_MAX; //G is the steps required to reach each node
	int GModifier = 0; //Can be used for Terrain movement cost in the future
	float GetH(FIntPoint EndPos)const {return (EndPos - XYPos).SizeSquared();}
	
	void SetupNodeData(FAStarNodeData* CurrentNode, const FIntPoint Pos)
	{
		XYPos = Pos;
		UpdateNodeData(CurrentNode);
	}
	bool UpdateNodeData(FAStarNodeData* CurrentNode); //Returns true if we updated the data of the node
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FGAI_2_API UFGAStarComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFGAStarComponent();

//User-facing Methods
public:
	//returns false if path cannot be found. Returns path in form of 2D Coordinates on the Grid
	UFUNCTION(BlueprintCallable)
	bool FindPathXY(const AFGGridActor* Grid, const FIntPoint StartPos, const FIntPoint InEndPos, TArray<FIntPoint>& OutPath);

//Internal Methods, mostly for refactoring and clean reading
protected:
	bool SearchGrid(const AFGGridActor* Grid, const FIntPoint StartPos, TArray<FIntPoint>& OutPath);
	TArray<FIntPoint> GetAllowedNeighbours(const AFGGridActor* Grid, const FIntPoint Pos) const;
	void HandleNeighbours(FAStarNodeData* CurrentNode, const AFGGridActor* Grid, const TArray<FIntPoint> ValidNeighboursPos);
	void AddNeighboursToVisit(TArray<int> NeighboursToAdd);
	TArray<FIntPoint> MakePathFromEndRecursive(const FAStarNodeData* Node, const AFGGridActor* Grid);

//Tweakable Variables
public:
	UPROPERTY(EditAnywhere)
	int MaxNodesToSearch = -1; //In case we want to search really large grid
	
//Internal Variables
protected:
	TArray<int> NodesToVisit; //Points to nodes in Map Created
	TSet<int> VisitedIndexes;
	TMap<int, FAStarNodeData> Created; //using this we keep track of which Nodes have already been created

	FIntPoint EndPos;
	int AmountNodesSearched = 0;
};
