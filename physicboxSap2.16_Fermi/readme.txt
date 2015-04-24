Usage:
m_sap->collision(interval, ParticlePos(), Pairs(), PairEntries(), &nPairs, &nPEntries);
Interal is equal to max(extent.x, extent.y, extent.z);
And the pair entries are like this, if the pair list is
(0,3)(0,5)(0,7)(2,1)(2,4)(4,1)(4,2)...
The pair entries for this is
0, 3, 5, ...
"0" is the start of pairs for idx0, "3" is start for idx2, "5" is start for idx4.  