#pragma once


struct PlayerComponent
{
    PlayerComponent(float _playerSpeed = 1.0f) : m_playerSpeed(_playerSpeed)
    {

    }
    float m_playerSpeed = 1.0f;
};