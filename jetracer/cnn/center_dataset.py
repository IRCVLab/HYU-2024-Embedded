import numpy as np
import os
import PIL.Image

import torch
import torch.utils.data
import torchvision.transforms as transforms

TRAIN_TRANSFORMS = transforms.Compose([
    # transforms.ColorJitter(0.2, 0.2, 0.2, 0.2),
    transforms.Resize((224, 224)),    
    transforms.ToTensor(),
    transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])
])

TEST_TRANSFORMS = transforms.Compose([
    transforms.Resize((224, 224)),    
    transforms.ToTensor(),
    transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225])
])


class CenterDataset(torch.utils.data.Dataset):
    def __init__(self, root_dir, random_hflip=True, transform=TRAIN_TRANSFORMS):
        super(CenterDataset, self).__init__()
        self.root_dir = root_dir
        self.random_hflip = random_hflip
        self.transform = transform
                
        with open(os.path.join(root_dir, 'annotation.txt'), 'r') as f:
            self.data = [line.split() for line in f.readlines()]
            # self.data = [(image_filename: (xpos, ypos) for image_filename, xpos, ypos in f.readlines()}
                        
    def __len__(self):
        return len(self.data)
    
    def __getitem__(self, idx):        
        filename, xpos, ypos = self.data[idx]
        xpos = int(xpos)
        ypos = int(ypos)

        image = PIL.Image.open(filename)
        width = image.width
        height = image.height

        # image = cv2.imread(filename, cv2.IMREAD_COLOR)
        # image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
        # height, width = image.shape[:2]

        # image = PIL.Image.fromarray(image)
        # width = image.width
        # height = image.height
        if self.transform is not None:
            image = self.transform(image)
        
        x = 2.0 * (xpos / width - 0.5) # map to [-1, +1]
        y = 2.0 * (ypos / height - 0.5) # map to [-1, +1]
        
        if self.random_hflip and float(np.random.random(1)) > 0.5:
            image = torch.flip(image, [-1])
            x = -x
            
        return image, torch.Tensor([x, y])
