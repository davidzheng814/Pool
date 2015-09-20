//
//  TableView.m
//  PocketPool
//
//  Created by Catheryn Li on 9/19/15.
//  Copyright (c) 2015 TeamAbs. All rights reserved.
//

#import "TableView.h"

@implementation TableView

- (void)setUp {
    // Render background image
//    NSLog(@"TableView setUp");
    UIImage *backgroundImage = [UIImage imageNamed:@"table.png"];
    UIImageView *backgroundView = [[UIImageView alloc] initWithImage:backgroundImage];
    [self addSubview:backgroundView];
    [self sendSubviewToBack:backgroundView];
}

- (void)renderImageWithBallPositions:(NSArray *)ballPositions {
//    NSLog(@"TableView renderImage called");
    for (int i = 0; i < [ballPositions count]; i++) {
//        NSLog(@"Drawing ball %d", i);
        NSValue *positionVal = ballPositions[i];
        CGPoint position = [positionVal CGPointValue];
        if (position.x > 0 && position.y > 0) {
            NSString *ballName = [NSString stringWithFormat: @"ball%d.png", i];
            UIImageView *ballImage =[[UIImageView alloc] initWithFrame:CGRectMake(position.x, position.y, 30, 30)];
            ballImage.image = [UIImage imageNamed:ballName];
            [self addSubview:ballImage];
        }
    }
}

@end