export module [project_name];

export import Scythe;

export class [project_name]Game final : public Game
{
#ifdef IMGUI_ENABLED
    void DrawDebug(bool* opened);
#endif
public:
    [project_name]Game();
    
    void Init() override;
    void Update(double dt) override;
};
