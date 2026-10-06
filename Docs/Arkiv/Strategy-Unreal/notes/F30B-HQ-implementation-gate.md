# F30B Higher HQ implementation gate

The higher-command design is committed, but runtime implementation must only start after the current F30A NullReference regressions are removed and Unity is re-tested.

Required runtime fixes before F30B code:

1. Null-safe `Regiment.FindNearestEnemyInFireArc` when BattleManager/Regiments is not ready or is being rebuilt.
2. Null-safe `PrototypeChargeTargeting09F29K.CaptureLegacyTargetPick` if the legacy charge engine/reflection state is unavailable during scene/bootstrap transitions.
3. Re-run Unity QA with no repeated NullReference spam.

Then implement physical Division HQ and Brigade HQ, OOB integration, command selection and cavalry attachment parentage.
