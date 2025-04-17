export module Editor:EditorDelegates;

import Common;
import Tools;

export class Scene;

export namespace EditorDelegates
{
    Delegate<bool> PlayInEditorDelegate;
    Delegate<const Scene&> OnSceneSaved;
    Delegate<IAssetRef*> OnAssetSelected;
}
