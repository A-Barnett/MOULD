
#include "SlimePlayer.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/MeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/Texture2D.h"
#include "Rendering/Texture2DResource.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/TextBlock.h"
#include "Camera/CameraActor.h"

ASlimePlayer::ASlimePlayer()
{


    // Disable the default capsule collider
    GetCapsuleComponent()->SetCapsuleRadius(15.0f);
    GetCapsuleComponent()->SetCapsuleHalfHeight(15.0f);

    // Turn off gravity
    GetCharacterMovement()->GravityScale = 0.0f;
    
    
    // Create and set up the camera component
    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    float targetx = fminf(-400.0f * powf((-scale + 2), 3) - 800, -800.0f);
    float targety = fmaxf(100.0f * powf((-scale + 2), 3) - 1000, -1000.0f);
    CameraComponent->SetRelativeLocation(FVector(targetx, 0, targety));
    CameraComponent->SetupAttachment(RootComponent); // Attach camera to the root component
    CameraComponent->SetRelativeRotation(FRotator(pitch, 0.0f, 0.0f)); // Pitch down 15 degrees
    bUseControllerRotationYaw = true;  // Player rotates with yaw input
    bUseControllerRotationPitch = false; // Only camera moves for pitch
    bUseControllerRotationRoll = false;

  //  SpotLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("LightComponent"));
   // SpotLight->SetupAttachment(RootComponent); // Attach camera to the root component


    GetCharacterMovement()->bUseSeparateBrakingFriction = true;
    GetCharacterMovement()->BrakingFriction = 10.0f; // Adjust friction for responsiveness
    GetCharacterMovement()->BrakingDecelerationFlying = 2048.0f; // High deceleration for quick stopping
    GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f;
    GetCharacterMovement()->AirControl = 1.0f; // Full control in air
    GetCharacterMovement()->AirControlBoostMultiplier = 1.0f; // Disable boosts
    GetCharacterMovement()->AirControlBoostVelocityThreshold = 0.0f; // Remove thresholds for boost
    GetCharacterMovement()->MaxFlySpeed = 800.0f; // Adjust to desired speed
    GetCharacterMovement()->MaxWalkSpeed = 800.0f; // Ensure consistency
    GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &ASlimePlayer::OnOverlapBegin);
    // Tick enabled

    EditableTexture = CreateEditableTexture(2000, 2000);
    PrimaryActorTick.bCanEverTick = true;
}

void ASlimePlayer::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent,AActor* OtherActor,UPrimitiveComponent* OtherComp,int32 OtherBodyIndex,bool bFromSweep,const FHitResult& SweepResult)
{
    if (OtherComp->ComponentHasTag("apple")) {
        targetScale += 0.07f;
        GetCapsuleComponent()->SetCapsuleRadius(15.0f+(targetScale*80));
        GetCharacterMovement()->MaxWalkSpeed += 25.0f;
        OtherActor->Destroy();
    }
}



void ASlimePlayer::BeginPlay()
{
    Super::BeginPlay();
    QueryParams.AddIgnoredActor(this);

    PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);

    if (HUDWidgetClass)
    {
        HUDWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), HUDWidgetClass);
        if (HUDWidgetInstance)
        {
            AreaTextBox = Cast<UTextBlock>(HUDWidgetInstance->GetWidgetFromName("Area"));
            TimeTextBox = Cast<UTextBlock>(HUDWidgetInstance->GetWidgetFromName("Time"));
        }
    }

    if (StartWidgetClass)
    {
        StartWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), StartWidgetClass);
        if (StartWidgetInstance)
        {
            StartButton = Cast<UButton>(StartWidgetInstance->GetWidgetFromName("Start"));
            if (StartButton) StartButton->OnClicked.AddDynamic(this, &ASlimePlayer::GameStart);

            ReverseButton = Cast<UButton>(StartWidgetInstance->GetWidgetFromName("Reverse"));
            if (ReverseButton) ReverseButton->OnClicked.AddDynamic(this, &ASlimePlayer::GameReverse);

            ExitButton = Cast<UButton>(StartWidgetInstance->GetWidgetFromName("Exit"));
            if (ExitButton) ExitButton->OnClicked.AddDynamic(this, &ASlimePlayer::Exit);
        }
        StartWidgetInstance->AddToViewport();
    }

    // Setup End Screen UI
    if (EndWidgetClass)
    {
        EndWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), EndWidgetClass);
        if (EndWidgetInstance)
        {
            MenuButton = Cast<UButton>(EndWidgetInstance->GetWidgetFromName("Menu"));
            if (MenuButton) MenuButton->OnClicked.AddDynamic(this, &ASlimePlayer::Menu);

            ExitEndButton = Cast<UButton>(EndWidgetInstance->GetWidgetFromName("Exit"));
            if (ExitEndButton) ExitEndButton->OnClicked.AddDynamic(this, &ASlimePlayer::Exit);

            ScoreTextBox = Cast<UTextBlock>(EndWidgetInstance->GetWidgetFromName("Score"));
        }
    }

    // Setup Pause Menu UI (but do NOT set Pause to anything yet)
    if (PauseWidgetClass)
    {
        PauseWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), PauseWidgetClass);
        if (PauseWidgetInstance)
        {
            ResumeButton = Cast<UButton>(PauseWidgetInstance->GetWidgetFromName("Resume"));
            if (ResumeButton) ResumeButton->OnClicked.AddDynamic(this, &ASlimePlayer::Pause);

            MenuPauseButton = Cast<UButton>(PauseWidgetInstance->GetWidgetFromName("Menu"));
            if (MenuPauseButton) MenuPauseButton->OnClicked.AddDynamic(this, &ASlimePlayer::Menu);

            ExitPauseButton = Cast<UButton>(PauseWidgetInstance->GetWidgetFromName("Exit"));
            if (ExitPauseButton) ExitPauseButton->OnClicked.AddDynamic(this, &ASlimePlayer::Exit);
        }
    }


    TArray<AActor*> camActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("cam"), camActors);
    if (camActors.Num() > 0)
    {
        // Cast to CameraActor
       SceneCamActor = Cast<ACameraActor>(camActors[0]);
        if (SceneCamActor)
        {
            if (PlayerController)
            {
                PlayerController->SetViewTarget(SceneCamActor);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find SceneCam in the level"));
    }

    // Show mouse cursor and set input mode to UI
    if (PlayerController)
    {
        PlayerController->bShowMouseCursor = true;
        PlayerController->SetInputMode(FInputModeUIOnly());
    }

    // Find all actors with the "floor" tag
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("floor"), FoundActors);


    // Iterate over all found actors
    for (AActor* FloorActor : FoundActors)
    {
            // Access the static mesh component of the floor
            UStaticMeshComponent* FloorMesh = FloorActor->FindComponentByClass<UStaticMeshComponent>();
            if (FloorMesh)
            {
                // Get the number of materials on this mesh
                int32 MaterialCount = FloorMesh->GetNumMaterials();
                for (int32 Index = 0; Index < MaterialCount; ++Index)
                {
                    // Get the material instance
                    UMaterialInterface* Material = FloorMesh->GetMaterial(Index);
                    if (Material)
                    {
                        // Create a dynamic material instance and assign it back to the mesh
                        UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
                        if (DynamicMaterial)
                        { 
                            FloorMesh->SetMaterial(Index, DynamicMaterial);
                            DynamicMaterialsArray.Add(DynamicMaterial);
                            UTexture* getter;
                            DynamicMaterial->GetTextureParameterValue(FName("PlayerTex"), getter);
                            getter = EditableTexture;
                            DynamicMaterial->SetTextureParameterValue(FName("PlayerTex"), getter);
                        }
                    }
                }
            
        }
    }

}


void ASlimePlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UpdateTextureWithPlayerPosition();
    if (!gameStarted || paused) return;
    time -= DeltaTime;
    if (time < 0) {
        GameEnded();
        return;
    }
    if (scale != targetScale) {
        scale = FMath::FInterpTo(scale, targetScale, DeltaTime, 1.5f);
    }
    UE_LOG(LogTemp, Display, TEXT("Scale %f"),scale);
    int32 max = 1400 * 1400 * 255;
    float areaPercent = (float(totalRed) / float(max)) * 100.0f;
    if (AreaTextBox && TimeTextBox)
    {
        FString AreaText = FString::Printf(TEXT("Area: %.2f%%"), areaPercent); // Format to 2 decimal places
        AreaTextBox->SetText(FText::FromString(AreaText));
        FString TimeText = FString::Printf(TEXT("Time: %.2fs"), time); // Format to 2 decimal places
        TimeTextBox->SetText(FText::FromString(TimeText));
    }

    float targetx = fminf(-500.0f * scale - 800, -800.0f);
    float targety = fmaxf(150.0f * scale -700, -800.0f);
    CameraComponent->SetRelativeLocation(FVector(targetx,0, targety));
    CameraComponent->SetRelativeRotation(FRotator(pitch, 0.0f, 0.0f)); // Pitch down 15 degrees
    // Perform a line trace downward from the player's current location
    FVector Start = GetActorLocation();
    FVector End = Start - FVector(0.0f, 0.0f, 8000.0f); // Raycast 5000 units downward
    FHitResult HitResult;


    if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
    {

  
        // If we hit something, calculate the target height
        float TargetHeight = HitResult.Location.Z + floorHeight;
        if (TargetHeight >= 1800.0f) return;

        FVector CurrentLocation = GetActorLocation();
        float HeightDifference = FMath::Abs(CurrentLocation.Z - TargetHeight);
        FHitResult HitResultUp;
        FVector EndUp = Start;
        EndUp.Z = TargetHeight; // Raycast 5000 units downward
        if (HeightDifference > 10.0f)
        {
            if (!GetWorld()->LineTraceSingleByChannel(HitResultUp, Start, EndUp, ECC_Visibility, QueryParams)) {
                CurrentLocation.Z = FMath::FInterpTo(CurrentLocation.Z, TargetHeight, DeltaTime, 2.5f);
                SetActorLocation(CurrentLocation, false, nullptr, ETeleportType::TeleportPhysics);
            }
        }
    }

}

void ASlimePlayer::GameStart() {
    gameStarted = true;
    HUDWidgetInstance->AddToViewport();
    StartWidgetInstance->RemoveFromViewport();
    PlayerController->SetViewTarget(this);
    UE_LOG(LogTemp, Display, TEXT("START GAME"));
    // Hide mouse cursor and set input mode to gameplay
    if (PlayerController)
    {
        PlayerController->bShowMouseCursor = false;
        PlayerController->SetInputMode(FInputModeGameOnly());
    }
    // Update material parameters for all dynamic materials
    for (UMaterialInstanceDynamic* mat : DynamicMaterialsArray)
    {
        mat->SetScalarParameterValue(FName("Reverse"), 0);

    }

}

void ASlimePlayer::GameReverse() {
    gameStarted = true;
    HUDWidgetInstance->AddToViewport();
    StartWidgetInstance->RemoveFromViewport();
    PlayerController->SetViewTarget(this);
    UE_LOG(LogTemp, Display, TEXT("START GAME"));
    // Hide mouse cursor and set input mode to gameplay
    if (PlayerController)
    {
        PlayerController->bShowMouseCursor = false;
        PlayerController->SetInputMode(FInputModeGameOnly());
    }

    // Update material parameters for all dynamic materials
    for (UMaterialInstanceDynamic* mat : DynamicMaterialsArray)
    {
        mat->SetScalarParameterValue(FName("Reverse"), 1);

    }

}

void ASlimePlayer::GameEnded() {
    gameStarted = false;
    HUDWidgetInstance->RemoveFromViewport();
    StartWidgetInstance->RemoveFromViewport();
    PauseWidgetInstance->RemoveFromViewport();
    EndWidgetInstance->AddToViewport();

    PlayerController->SetViewTarget(SceneCamActor);
    UE_LOG(LogTemp, Display, TEXT("END GAME"));
    // Hide mouse cursor and set input mode to gameplay
    if (PlayerController)
    {
        PlayerController->bShowMouseCursor = true;
        PlayerController->SetInputMode(FInputModeUIOnly());
    }
    int32 max = 1400 * 1400 * 255;
    float areaPercent = (float(totalRed) / float(max)) * 100.0f;
    if (ScoreTextBox)
    {
        FString ScoreText = FString::Printf(TEXT("%.2f%%"), areaPercent); // Format to 2 decimal places
        ScoreTextBox->SetText(FText::FromString(ScoreText));
    }

}


void ASlimePlayer::Exit()
{
    UWorld* World = GetWorld();
    APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;

    if (PC)
    {
        UKismetSystemLibrary::QuitGame(World, PC, EQuitPreference::Quit, false);
    }
}


void ASlimePlayer::Menu()
{
    UWorld* World = GetWorld();
    if (World)
    {
        FName CurrentLevel = FName(*UGameplayStatics::GetCurrentLevelName(World));
        UGameplayStatics::OpenLevel(World, CurrentLevel);
    }
}

void ASlimePlayer::Pause() {
    paused = !paused;
    if (paused) {
        HUDWidgetInstance->RemoveFromViewport();
        StartWidgetInstance->RemoveFromViewport();
        PauseWidgetInstance->AddToViewport();
        EndWidgetInstance->RemoveFromViewport();
        if (PlayerController)
        {
            PlayerController->bShowMouseCursor = true;
            PlayerController->SetInputMode(FInputModeUIOnly());
        }
    }
    else {
        HUDWidgetInstance->AddToViewport();
        StartWidgetInstance->RemoveFromViewport();
        PauseWidgetInstance->RemoveFromViewport();
        EndWidgetInstance->RemoveFromViewport();
        if (PlayerController)
        {
            PlayerController->bShowMouseCursor = false;
            PlayerController->SetInputMode(FInputModeGameOnly());
        }
    }

}



void ASlimePlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);


    PlayerInputComponent->BindAction("Pause", IE_Pressed, this, &ASlimePlayer::Pause);
    PlayerInputComponent->BindAxis("MoveForward", this, &ASlimePlayer::MoveForward);
    PlayerInputComponent->BindAxis("MoveRight", this, &ASlimePlayer::MoveRight);
    PlayerInputComponent->BindAxis("ChangeScale", this, &ASlimePlayer::ChangeScale);
    // Bind look controls
    PlayerInputComponent->BindAxis("Turn", this, &ASlimePlayer::Turn); // Yaw
    PlayerInputComponent->BindAxis("LookUp", this, &ASlimePlayer::LookUp); // Pitch
}

void ASlimePlayer::MoveForward(float Value)
{
    AddMovementInput(GetActorForwardVector(), Value);
}

void ASlimePlayer::MoveRight(float Value)
{
    AddMovementInput(GetActorRightVector(), Value);
}

void ASlimePlayer::ChangeScale(float Value)
{
    scale += (0.02f * Value);
    scale = std::clamp(scale, 0.2f, 5.0f);

}

void ASlimePlayer::Turn(float Value)
{
    AddControllerYawInput(Value); // Rotate player
}

void ASlimePlayer::LookUp(float Value)
{
    pitch += (Value);
    pitch = std::clamp(pitch, -90.0f, 70.0f);
}

UTexture2D* ASlimePlayer::CreateEditableTexture(int32 Width, int32 Height)
{
    // Create a new 2D texture
    UTexture2D* NewTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);

    if (!NewTexture)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create texture!"));
        return nullptr;
    }

    // Prevent it from being garbage collected
    NewTexture->AddToRoot();

    // Set the texture settings
   // NewTexture->MipGenSettings = TMGS_NoMipmaps;
    NewTexture->SRGB = true; // Keep it in sRGB for color accuracy

    // Lock the texture to modify pixel data
    FTexture2DMipMap& Mip = NewTexture->GetPlatformData()->Mips[0];
    void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);

    // Initialize all pixels to black (RGBA = 0,0,0,0)
    FMemory::Memset(TextureData, 0, Mip.BulkData.GetBulkDataSize());

    // Unlock and update the texture resource
    Mip.BulkData.Unlock();
    NewTexture->UpdateResource();

    return NewTexture;
}

void ASlimePlayer::UpdateTextureWithPlayerPosition()
{
    if (!EditableTexture) return;

    // Access the first mipmap (assumes no mipmaps were generated since we disabled them earlier)
    FTexture2DMipMap& Mip = EditableTexture->GetPlatformData()->Mips[0];
    void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);

    // Get texture dimensions
    int32 Width = Mip.SizeX;
    int32 Height = Mip.SizeY;

    uint8* PixelData = static_cast<uint8*>(TextureData);

    // Scale the player's world position to the texture's coordinate space
    FVector WorldPos = GetActorLocation(); // Get the player's world position

    // Map world position to texture space (normalize to [0, 1] before scaling)
    float NormalizedX = (WorldPos.X * 0.00005f + 0.5f); // Normalize to [0, 1] range
    float NormalizedZ = (WorldPos.Y * 0.00005f + 0.5f); // Normalize to [0, 1] range

    int32 CenterX = FMath::Clamp(static_cast<int32>(NormalizedX * Width), 0, Width - 1);
    int32 CenterY = FMath::Clamp(static_cast<int32>(NormalizedZ * Height), 0, Height - 1);

    int32 Radius = 100*scale;
    int32 StartFade = 10*scale;   
    for (int32 Y = -Radius; Y <= Radius; ++Y)
    {
        for (int32 X = -Radius; X <= Radius; ++X)
        {
            // Compute the distance from the center
            float total = (X * X + Y * Y);
            float Distance = FMath::Sqrt(total);

            // Skip pixels outside the radius
            if (Distance > Radius) continue;

            // Calculate fade factor (0 to 1, where 1 is fully red)
            float FadeFactor = 1.0f;
            if (Distance > StartFade)
            {
                FadeFactor = FMath::Clamp(1.0f - ((Distance - StartFade) / (Radius - StartFade)), 0.0f, 1.0f);
            }

            // Compute texture coordinates
            int32 PixelX = FMath::Clamp(CenterX + X, 0, Width - 1);
            int32 PixelY = FMath::Clamp(CenterY + Y, 0, Height - 1);

            // Calculate the pixel index (RGBA = 4 bytes per pixel)
            int32 PixelIndex = (PixelY * Width + PixelX) * 4;

            // Add to the red channel instead of overriding (up to 255)
            uint8 CurrentRed = PixelData[PixelIndex + 2]; // Current red value
            uint8 AddRed = static_cast<uint8>(FadeFactor * 255.0f); // Red intensity to add
            if (AddRed > CurrentRed) {
                totalRed += (AddRed - CurrentRed);
                PixelData[PixelIndex + 2] = FMath::Clamp(AddRed, 0, 255); // Set final red channel value
            }

            // Ensure alpha is fully opaque
            PixelData[PixelIndex + 3] = 255;
        }
    }

    // Unlock the texture after editing
    Mip.BulkData.Unlock();
    EditableTexture->UpdateResource(); // Apply changes to the GPU
}
