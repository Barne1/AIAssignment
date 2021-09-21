#include "FGAStarComponent.h"

#include "Grid/FGGridActor.h"

bool FAStarNodeData::UpdateNodeData(FAStarNodeData* CurrentNode)
{
	if(PreviousNode == nullptr || CurrentNode->G < PreviousNode->G)
	{
		PreviousNode = CurrentNode;
		G = PreviousNode->G + 1 + GModifier;
		return true;
	}
	return false;
}


UFGAStarComponent::UFGAStarComponent()
{
}

bool UFGAStarComponent::FindPathXY(const AFGGridActor* Grid, const FIntPoint StartPos, const FIntPoint InEndPos, TArray<FIntPoint>& OutPath)
{
	//Check that all requested values are correct
	if(Grid == nullptr || StartPos-InEndPos == 0 || !Grid->IsXYValid(StartPos.X, StartPos.Y) || !Grid->IsXYValid(InEndPos.X, InEndPos.Y))
		return false;

	//Clear all values
	AmountNodesSearched = 0;
	NodesToVisit.Empty();
	VisitedIndexes.Empty();
	Created.Empty();
	OutPath.Empty();
	EndPos = InEndPos;
	
	//Setup of First Node
	FAStarNodeData StartNode;
	StartNode.G = 1;
	StartNode.XYPos = StartPos;
	int Index = Grid->GetTileIndexFromXY(StartPos.X, StartPos.Y);

	Created.Add(Index, StartNode);
	NodesToVisit.Add(Index);

	//Start of search
	return SearchGrid(Grid, StartPos, OutPath);
}

bool UFGAStarComponent::SearchGrid(const AFGGridActor* Grid, const FIntPoint StartPos, TArray<FIntPoint>& OutPath)
{
	//Core loop
	while(NodesToVisit.Num() > 0)
	{
		//Node limit loop break
		if(MaxNodesToSearch > 0 && AmountNodesSearched >= MaxNodesToSearch)
		{
			return false;
		}
		AmountNodesSearched++;

		int CurrentNodeIndex = NodesToVisit[0];
		FAStarNodeData* CurrentNode = &Created[CurrentNodeIndex];
		
		if(CurrentNode->XYPos == EndPos) //if we found the end node
		{
			OutPath = MakePathFromEndRecursive(CurrentNode, Grid);
			return true;
		}
		
		TArray<FIntPoint> Neighbours = GetAllowedNeighbours(Grid, CurrentNode->XYPos);
		
		if(Neighbours.Num() > 0)
		{
			HandleNeighbours(CurrentNode, Grid, Neighbours);
		}
		
		NodesToVisit.RemoveSingle(CurrentNodeIndex);
		VisitedIndexes.Add(CurrentNodeIndex);
	}
	return false;
}

TArray<FIntPoint> UFGAStarComponent::GetAllowedNeighbours(const AFGGridActor* Grid, const FIntPoint Pos) const
{
	//Tile indices of the final tiles allowed
	TArray<FIntPoint> FinalNeighbours;

	//Simple Directions
	const FIntPoint Up(0,1);
	const FIntPoint Down(0,-1);
	const FIntPoint Right(1,0);
	const FIntPoint Left(-1,0);

	//This solution allows for easy additions of other directions in the future
	TArray<FIntPoint> PotentialNeighbours = {Up,Down,Left,Right};

	for(auto Neighbor : PotentialNeighbours)
	{
		FIntPoint NeighbourPos = Pos + Neighbor;
		int NeighbourIndex = Grid->GetTileIndexFromXY(NeighbourPos.X, NeighbourPos.Y);

		//Make sure tile is valid in all ways
		bool bInsideGrid = Grid->IsXYValid(NeighbourPos.X, NeighbourPos.Y);
		bool bAlreadyVisited = VisitedIndexes.Contains(NeighbourIndex);
		bool bIsBlocked = Grid->TileList[NeighbourIndex].bBlock;
		
		if(bInsideGrid && !bAlreadyVisited && !bIsBlocked)
		{
			//If valid its added to a list of allowed neighbours
			FinalNeighbours.Add(NeighbourPos);
		}
	}
	return FinalNeighbours;
}


void UFGAStarComponent::HandleNeighbours(FAStarNodeData* CurrentNode, const AFGGridActor* Grid, const TArray<FIntPoint> ValidNeighboursPos)
{
	//This is the list of Neighbours about to be added in to the ToVisit queue/list
	//It will also include Neighbours whose Heuristics value we adjusted, since they are removed to be readded
	TArray<int> ToAddNeighbours;
	
	for(auto NeighbourPos : ValidNeighboursPos)
	{
		int NeighbourIndex = Grid->GetTileIndexFromXY(NeighbourPos.X, NeighbourPos.Y);
		//If we already been to the node, ignore it
		if(VisitedIndexes.Contains(NeighbourIndex))
		{
			continue;
		}

		//If we haven't looked at the node at all, create it anew and prepare to add it
		if(!Created.Contains(NeighbourIndex))
		{
			FAStarNodeData NeighbourNodeData(CurrentNode, NeighbourPos);
			Created.Add(NeighbourIndex, NeighbourNodeData);
			ToAddNeighbours.Add(NeighbourIndex);
		}
		else //If we have looked at the node from another Neighbour
		{
			//Look if this Heuristic value is lower than the previous Neighbour
			bool bUpdated = Created[NeighbourIndex].UpdateNodeData(CurrentNode);
			
			if(bUpdated) //if we updated the Heuristics value of the node we remove it and prepare to re-add it to a better position
			{
				NodesToVisit.RemoveSingle(NeighbourIndex);
				ToAddNeighbours.Add(NeighbourIndex);
			}
		}
	}
	//If we have any to add to the list, add them in
	if(ToAddNeighbours.Num() > 0)
	{
		AddNeighboursToVisit(ToAddNeighbours);
	}
}

void UFGAStarComponent::AddNeighboursToVisit(TArray<int> NeighboursToAdd)
{
	for (auto NeighbourIndex : NeighboursToAdd)
	{
		//this check is used if we looped through the entire list (and therefore need to add our neighbour to the end)
		bool bFoundCorrectPlace = false;

		for (int i = 0; i < NodesToVisit.Num(); ++i)
		{
			int NodeIndex = NodesToVisit[i];
			const FAStarNodeData* NodeData = &Created[NodeIndex];
			const FAStarNodeData* NeighbourData = &Created[NeighbourIndex];

			float NodeF = NodeData->GetF(EndPos);
			float NeighbourF = NeighbourData->GetF(EndPos);

			//compare F values to see if we found the place to insert our Neighbour
			if(NeighbourF < NodeF)
			{
				NodesToVisit.Insert(NeighbourIndex, i);
				bFoundCorrectPlace = true;
				break;
			}
		}
		//this will happen even if the for loop above doesnt happen (due to the list being empty)
		if(!bFoundCorrectPlace) //add at the end if we didnt find any smaller F values
		{
			NodesToVisit.Add(NeighbourIndex);
		}
	}
}

TArray<FIntPoint> UFGAStarComponent::MakePathFromEndRecursive(const FAStarNodeData* Node, const AFGGridActor* Grid)
{
	//Bottom of the recusion
	if(Node->PreviousNode == nullptr)
	{
		TArray<FIntPoint> Path;
		Path.Add(Node->XYPos);
		return Path;
	}
	
	TArray<FIntPoint> Path = MakePathFromEndRecursive(Node->PreviousNode, Grid);
	Path.Add(Node->XYPos);
	return Path;
}
