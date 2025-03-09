#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

#include "SlimePlayer.generated.h"


class USphereComponent;
class UCameraComponent;
class UMaterialInstanceDynamic;

UCLASS()
class SLIME_API ASlimePlayer : public ACharacter
{
    GENERATED_BODY()

public:
    // Constructor
    ASlimePlayer();

protected:
    // Sphere Collider
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
    USphereComponent* SphereCollider;

    // Camera Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCameraComponent* CameraComponent;

    // Camera Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCameraComponent* SceneCam;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> HUDWidgetClass;

    UPROPERTY()
    UUserWidget* HUDWidgetInstance;

    UPROPERTY()
    UTextBlock* AreaTextBox;

    UPROPERTY()
    UTextBlock* TimeTextBox;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> StartWidgetClass;

    UPROPERTY()
    UUserWidget* StartWidgetInstance;

    UPROPERTY()
    UButton* StartButton;
    UPROPERTY()
    UButton* ReverseButton;
    UPROPERTY()
    UButton* ExitButton;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> EndWidgetClass;

    UPROPERTY()
    UUserWidget* EndWidgetInstance;
    UPROPERTY()
    UTextBlock* ScoreTextBox;

    UPROPERTY()
    UButton* MenuButton;

    UPROPERTY()
    UButton* ExitEndButton;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> PauseWidgetClass;

    UPROPERTY()
    UUserWidget* PauseWidgetInstance;

    UPROPERTY()
    UButton* ResumeButton;

    UPROPERTY()
    UButton* MenuPauseButton;

    UPROPERTY()
    UButton* ExitPauseButton;

    UPROPERTY()
    APlayerController* PlayerController;

    UPROPERTY()
    TArray<UMaterialInstanceDynamic*> DynamicMaterialsArray;

    FCollisionQueryParams QueryParams;

    float floorHeight = 1500.0f;
    float time = 75.0f;
    float speed = 10.0f;
    float scale = 0.5f;
    float targetScale = 0.5f;
    float pitch = -25.0f;
    int32 totalRed = 0;
    bool gameStarted;
    bool paused = false;

    UTexture2D* EditableTexture;
    ACameraActor* SceneCamActor;
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    UFUNCTION()
    void OnOverlapBegin(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );


    void MoveForward(float Value);
    void MoveRight(float Value);
    void ChangeScale(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    UTexture2D* CreateEditableTexture(int32 Width, int32 Height);
    UFUNCTION()
    void GameStart();
    void GameEnded();
    UFUNCTION()
    void GameReverse();
    UFUNCTION()
    void Exit();
    UFUNCTION()
    void Menu();
    UFUNCTION()
    void Pause();
    void UpdateTextureWithPlayerPosition();
};
